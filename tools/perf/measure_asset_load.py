"""Cold scene load/encode probe using already-built Release libraries; no production target changes."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import subprocess

ROOT = Path(__file__).resolve().parents[2]
PROBE = r'''#include <gloom/assets/asset.hpp>
#include <gloom/assets/gltf_importer.hpp>
#include <gloom/core/clock.hpp>
#include <gloom/core/allocation_profile.hpp>
#include <stdio.h>
#include <stdlib.h>
#ifdef GLOOM_ALLOCATION_PROFILE
extern gloom::uint64 entity_live_bytes, entity_peak_bytes;
#endif
int main(int argc, char** argv) {
    if (argc != 3) return 2;
    gloom::assets::VirtualFileSystem fs;
    static_cast<void>(fs.mount("cache", std::filesystem::path{argv[1]}));
    const std::expected<gloom::assets::VirtualPath, std::string> path = gloom::assets::VirtualPath::parse("cache:/scene.gasset");
    const std::expected<std::vector<std::byte>, std::string> bytes = fs.read(*path);
    if (!bytes) return 3;
    const std::expected<gloom::assets::CookedAsset, std::string> envelope = gloom::assets::decode_cooked_asset(*bytes);
    if (!envelope) return 4;
    const std::expected<gloom::assets::ImportedScene, std::string> scene = gloom::assets::decode_imported_scene(envelope->payload);
    if (!scene) return 5;
    gloom::uint32 repetitions = static_cast<gloom::uint32>(atoi(argv[2]));
#ifdef GLOOM_ALLOCATION_PROFILE
    repetitions = 1;
#endif
    gloom::uint64 checksum = 0;
    for (gloom::uint32 stage = 0; stage < 2; ++stage) {
#ifdef GLOOM_ALLOCATION_PROFILE
        const gloom::uint64 baseline = entity_live_bytes;
        entity_peak_bytes = baseline;
        gloom::allocation_profile_reset();
        gloom::allocation_profile_window(true);
#endif
        const gloom::uint64 started = gloom::performance_clock();
        {
            GLOOM_PROFILE_SCOPE(gloom::AllocationPhase::assets);
            for (gloom::uint32 i = 0; i < repetitions; ++i) {
                if (!stage) {
                    const std::expected<std::vector<std::byte>, std::string> input = fs.read(*path);
                    if (!input) return 6;
                    const std::expected<gloom::assets::CookedAsset, std::string> asset = gloom::assets::decode_cooked_asset(*input);
                    if (!asset) return 7;
                    const std::expected<gloom::assets::ImportedScene, std::string> loaded = gloom::assets::decode_imported_scene(asset->payload);
                    if (!loaded) return 8;
                    checksum += loaded->nodes.size() + loaded->primitives.size() + asset->payload.size();
                } else {
                    const std::vector<std::byte> encoded = gloom::assets::encode_imported_scene(*scene);
                    checksum += encoded.size() + gloom::assets::fingerprint(encoded);
                }
            }
        }
        const double ns = (gloom::performance_clock() - started) * 1e9 / gloom::performance_frequency() / repetitions;
#ifdef GLOOM_ALLOCATION_PROFILE
        gloom::allocation_profile_window(false);
        const gloom::AllocationStats stats = gloom::allocation_profile_read(gloom::AllocationPhase::assets);
        printf("stage=%u calls=%llu bytes=%llu peak=%llu live_delta=%llu\n", stage, stats.calls, stats.bytes, entity_peak_bytes-baseline, entity_live_bytes-baseline);
#else
        printf("stage=%u ns=%.3f\n", stage, ns);
#endif
    }
    printf("checksum=%llu\n",checksum);
}
'''
COUNTER = r'''#include <gloom/core/allocation_profile.hpp>
#include <stdlib.h>
gloom::uint64 entity_live_bytes=0,entity_peak_bytes=0;
namespace {struct alignas(16) Header {size_t bytes;};}
void* operator new(size_t bytes) {
    Header* p=static_cast<Header*>(malloc(sizeof(Header)+bytes));p->bytes=bytes;
    entity_live_bytes+=bytes;
    if(entity_live_bytes>entity_peak_bytes)entity_peak_bytes=entity_live_bytes;
    gloom::allocation_profile_record(bytes);return p+1;
}
void* operator new[](size_t bytes) {return operator new(bytes);}
void operator delete(void* address) noexcept {
    if(!address)return;
    Header* p=static_cast<Header*>(address)-1;entity_live_bytes-=p->bytes;free(p);
}
void operator delete[](void* address) noexcept {operator delete(address);}
void operator delete(void* address,size_t) noexcept {operator delete(address);}
void operator delete[](void* address,size_t) noexcept {operator delete(address);}
'''

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--files', type=Path, required=True, help='small/factory/hound directories containing scene.gasset')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--libraries', type=Path, default=ROOT/'build/windows-vs/Release')
    parser.add_argument('--headers', type=Path, default=ROOT/'include')
    args = parser.parse_args()
    output = args.output.resolve()
    output.mkdir(parents=True, exist_ok=True)
    (output/'probe.cpp').write_text(PROBE, encoding='utf-8')
    (output/'new.cpp').write_text(COUNTER, encoding='utf-8')
    cache = dict(line.split('=',1) for line in (ROOT/'build/windows-vs/CMakeCache.txt').read_text().splitlines()
                 if '=' in line and not line.startswith(('#','//')))
    instance = next(value for key,value in cache.items() if key.startswith('CMAKE_GENERATOR_INSTANCE:'))
    prefix = ROOT/'build/windows-vs/vcpkg_installed/x64-windows'
    cmake = f"""cmake_minimum_required(VERSION 3.28)
project(asset_load_probe LANGUAGES CXX)
find_package(fastgltf CONFIG REQUIRED)
find_package(Ktx CONFIG REQUIRED)
find_package(meshoptimizer CONFIG REQUIRED)
find_package(mikktspace CONFIG REQUIRED)
foreach(mode IN ITEMS normal memory)
 add_executable(probe_${{mode}} probe.cpp)
 target_compile_features(probe_${{mode}} PRIVATE cxx_std_23)
 target_compile_options(probe_${{mode}} PRIVATE /GR- /EHs-c- /utf-8 /wd4530)
 target_include_directories(probe_${{mode}} PRIVATE "{args.headers.resolve().as_posix()}" "{ROOT.as_posix()}/include")
 target_link_libraries(probe_${{mode}} PRIVATE "{args.libraries.resolve().as_posix()}/gloom_asset_pipeline.lib"
  "{args.libraries.resolve().as_posix()}/gloom_engine.lib" fastgltf::fastgltf KTX::ktx meshoptimizer::meshoptimizer mikktspace::mikktspace)
endforeach()
target_sources(probe_memory PRIVATE new.cpp "{ROOT.as_posix()}/src/core/allocation_profile.cpp")
target_compile_definitions(probe_memory PRIVATE GLOOM_ALLOCATION_PROFILE=1)
"""
    (output/'CMakeLists.txt').write_text(cmake, encoding='utf-8')
    def run(command, name):
        with (output/name).open('w',encoding='utf-8') as log:
            result = subprocess.run(['rtk','proxy',*command],cwd=ROOT,stdout=log,stderr=subprocess.STDOUT)
        if result.returncode: raise RuntimeError(f'{name}: exit {result.returncode}')
    run(['cmake','-S',str(output),'-B',str(output/'build'),'-G','Visual Studio 17 2022','-A','x64',
         '-DCMAKE_GENERATOR_INSTANCE='+instance,'-DCMAKE_PREFIX_PATH='+str(prefix)], 'configure.log')
    run(['cmake','--build',str(output/'build'),'--config','Release','--parallel','4'], 'build.log')
    env=os.environ.copy()
    env['PATH']=str(ROOT/'build/windows-vs/Release')+os.pathsep+env['PATH']
    rows=[]
    for mode in ['normal','memory']:
        exe=output/f'build/Release/probe_{mode}.exe'
        for case,repetitions in [('small',100),('factory',20),('hound',5)]:
            for trial in range(5 if mode=='normal' else 1):
                result=subprocess.run(['rtk','proxy',str(exe),str(args.files.resolve()/case),str(repetitions)],
                                      cwd=ROOT,env=env,text=True,capture_output=True)
                if result.returncode: raise RuntimeError((case,result.returncode,result.stdout,result.stderr))
                rows.append(dict(mode=mode,case=case,trial=trial,output=result.stdout))
            print(mode,case,rows[-1]['output'].strip(),flush=True)
    data=dict(probe_sha256=hashlib.sha256(PROBE.encode()).hexdigest(),
              fixtures={case:hashlib.sha256((args.files/case/'scene.gasset').read_bytes()).hexdigest()
                        for case in ['small','factory','hound']},
              executables={mode:hashlib.sha256((output/f'build/Release/probe_{mode}.exe').read_bytes()).hexdigest()
                           for mode in ['normal','memory']},
              libraries={name:hashlib.sha256((args.libraries/name).read_bytes()).hexdigest()
                         for name in ['gloom_engine.lib','gloom_asset_pipeline.lib']},runs=rows)
    (output/'measurements.json').write_text(json.dumps(data,indent=2)+'\n',encoding='utf-8')

if __name__=='__main__': main()
