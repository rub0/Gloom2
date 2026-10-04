from pathlib import Path
import subprocess, argparse, json, re, hashlib
ROOT = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser(description='Compare 1000 job batches with a historical scheduler (Windows/MSVC).')
parser.add_argument('--output', type=Path, required=True)
parser.add_argument('--before', default='8b96c71')
parser.add_argument('--profile', action='store_true')
args = parser.parse_args()
root=args.output.resolve(); root.mkdir(parents=True,exist_ok=True)
inc=root/'include/gloom/core'; inc.mkdir(parents=True,exist_ok=True)
for name,destination in [('include/gloom/core/job_system.hpp',inc/'job_system.hpp'),('include/gloom/core/subsystem.hpp',inc/'subsystem.hpp'),('src/core/job_system.cpp',root/'job_system.cpp')]:
 destination.write_bytes(subprocess.check_output(['rtk','proxy','git','show',args.before+':'+name], cwd=ROOT))
code='''#include <gloom/core/job_system.hpp>
#include <gloom/core/clock.hpp>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
using namespace gloom;
struct Increment {
    volatile LONG* count;
    void operator()() const noexcept { InterlockedIncrement(count); }
};
int compare(const void* a, const void* b) {
    return *static_cast<const uint64*>(a) < *static_cast<const uint64*>(b) ? -1 : *static_cast<const uint64*>(a) > *static_cast<const uint64*>(b) ? 1 : 0;
}
int main() {
    const uint32 worker_counts[] = {1,4};
    for (uint32 workers : worker_counts) {
        core::JobSystem jobs{{.worker_threads = workers}};
#ifdef CURRENT_SCHEDULER
        if (const char* error = jobs.start()) { fprintf(stderr, "%s\\n", error); return 1; }
#else
        jobs.start();
#endif
        core::TaskGroup warm = jobs.create_group(); volatile LONG count=0;
        for (uint32 job=0;job<3;++job) jobs.schedule(warm,Increment{&count}); jobs.wait(warm);
        uint64 ticks[1000];
#ifdef GLOOM_ALLOCATION_PROFILE
        allocation_profile_window(false); allocation_profile_reset(); allocation_profile_window(true);
#endif
        for (uint32 round=0;round<1000;++round) {
            uint64 started=performance_clock(); core::TaskGroup group=jobs.create_group();
            for (uint32 job=0;job<3;++job) jobs.schedule(group,Increment{&count});
            jobs.wait(group); ticks[round]=performance_clock()-started;
        }
#ifdef GLOOM_ALLOCATION_PROFILE
        allocation_profile_window(false); uint64 calls=0,bytes=0;
        for(uint32 phase=0;phase<static_cast<uint32>(AllocationPhase::count);++phase) {
            AllocationStats stats=allocation_profile_read(static_cast<AllocationPhase>(phase)); calls+=stats.calls;bytes+=stats.bytes;
        }
        printf("Job allocation: workers=%u rounds=1000 jobs=3000 own_new=%llu bytes=%llu\\n",workers,calls,bytes);
#endif
        qsort(ticks,1000,sizeof(uint64),compare);
        core::JobSystemMetrics metrics=jobs.metrics();
        printf("Job batch: workers=%u rounds=1000 median_us=%.3f p95_us=%.3f peak=%llu assisted=%llu scheduled=%llu completed=%llu wait_ms=%.3f\\n",workers,
            ticks[500]*1000000.0/performance_frequency(),ticks[950]*1000000.0/performance_frequency(),metrics.peak_queue_depth,metrics.caller_executed_jobs,
            metrics.scheduled_jobs,metrics.completed_jobs,metrics.wait_nanoseconds/1000000.0);
        if(count!=3003 || metrics.scheduled_jobs!=metrics.completed_jobs) return 1;
        jobs.stop();
    }
}
'''
(root/'control.cpp').write_text(code,encoding='utf-8')
path=ROOT.as_posix()
(root/'CMakeLists.txt').write_text(f'''cmake_minimum_required(VERSION 3.28)
project(job_control LANGUAGES CXX)
option(PROFILE "Allocation instrument" OFF)
option(BEFORE "Historical scheduler" ON)
if(BEFORE)
 add_executable(control control.cpp job_system.cpp "{path}/src/core/clock.cpp")
 target_include_directories(control PRIVATE include "{path}/include")
else()
 add_executable(control control.cpp "{path}/src/core/job_system.cpp" "{path}/src/core/clock.cpp")
 target_include_directories(control PRIVATE "{path}/include")
 target_compile_options(control PRIVATE /GR- /EHs-c-)
 target_compile_definitions(control PRIVATE CURRENT_SCHEDULER)
endif()
target_compile_features(control PRIVATE cxx_std_23)
target_compile_options(control PRIVATE /utf-8)
if(BEFORE)
 target_compile_options(control PRIVATE /EHsc)
endif()
if(PROFILE)
 target_sources(control PRIVATE "{path}/src/core/allocation_profile.cpp" "{path}/src/core/allocation_new.cpp")
 target_compile_definitions(control PRIVATE GLOOM_ALLOCATION_PROFILE)
endif()
''',encoding='utf-8')


def run(command, log):
    with log.open('w', encoding='utf-8') as output:
        result = subprocess.run(['rtk', 'proxy', *command], cwd=ROOT, stdout=output, stderr=subprocess.STDOUT, timeout=180)
    assert result.returncode == 0, (command, log.read_text(encoding='utf-8', errors='replace')[-3000:])

instance = json.loads((ROOT/'CMakePresets.json').read_text(encoding='utf-8'))['configurePresets'][0]['cacheVariables']['CMAKE_GENERATOR_INSTANCE']
executables = {}
for case in ('before', 'after'):
    build = root/case
    run(['cmake', '-S', str(root), '-B', str(build), '-G', 'Visual Studio 17 2022', '-A', 'x64',
         '-DCMAKE_GENERATOR_INSTANCE='+instance, '-DBEFORE='+('ON' if case == 'before' else 'OFF'),
         '-DPROFILE='+('ON' if args.profile else 'OFF')], root/(case+'-configure.log'))
    run(['cmake', '--build', str(build), '--config', 'Release', '--parallel', '4'], root/(case+'-build.log'))
    executables[case] = build/'Release/control.exe'

rows = []
for repetition in range(1 if args.profile else 7):
    for case, executable in executables.items():
        log = root/(case+'-'+str(repetition)+'.log')
        run([str(executable)], log)
        content = log.read_text(encoding='utf-8')
        for match in re.finditer(r'workers=(\d+) rounds=1000 median_us=([\d.]+) p95_us=([\d.]+) peak=(\d+) assisted=(\d+) scheduled=(\d+) completed=(\d+) wait_ms=([\d.]+)', content):
            values = match.groups()
            assert values[5] == values[6] == '3003', (case, values)
            rows.append(dict(case=case, repetition=repetition, workers=int(values[0]), median_us=float(values[1]), p95_us=float(values[2]),
                             peak=int(values[3]), assisted=int(values[4]), scheduled=int(values[5]), completed=int(values[6]), wait_ms=float(values[7])))
        if args.profile:
            for workers, calls, bytes_requested in re.findall(r'workers=(\d+) rounds=1000 jobs=3000 own_new=(\d+) bytes=(\d+)', content):
                if case == 'after': assert calls == bytes_requested == '0'
                rows[-2 if workers == '1' else -1].update(own_new=int(calls), bytes=int(bytes_requested))
assert len(rows) == (4 if args.profile else 28)
document = dict(before=args.before, after=subprocess.check_output(['rtk','proxy','git','rev-parse','HEAD'], cwd=ROOT, text=True).strip(),
                profile=args.profile,
                source_sha256={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in
                               ['src/core/job_system.cpp','include/gloom/core/job_system.hpp','include/gloom/core/fixed_function.hpp']},
                source_diff_sha256=hashlib.sha256(subprocess.check_output(['rtk','proxy','git','diff','HEAD'], cwd=ROOT)).hexdigest(),
                executables={case:hashlib.sha256(exe.read_bytes()).hexdigest() for case,exe in executables.items()}, runs=rows)
(root/'measurements.json').write_text(json.dumps(document, indent=2)+'\n', encoding='utf-8')
print(str(root/'measurements.json')+': '+str(len(rows))+' validated batch rows')
