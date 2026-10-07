# Probe reproducible del 118

Las cuatro variantes usan el mismo código. `before` enlaza librerías Release y
los tres headers públicos del commit 934b73d guardados antes de editar; `after`
usa los del 118. Recompilar ambas bases en directorios separados para repetir.
El include de la base guardada precede al include común. Dependencias instaladas
del proyecto; ninguna nueva. Ejecutar serialmente, con diagnóstico de runtime OFF.

Argumentos: raíz del cooked del 117 y URI fuente (`game:/tests/material_surface/scene.gltf`,
`game:/legacy/factory.gltf`, `game:/h07-v17/hound.gltf`). Cinco procesos normales por
variante/caso; contador en otro proceso. `measure_rss.py` consulta el hijo nativo
cada 2 ms; su peak WS es acumulado del proceso y el privado máximo es muestreado.
El receptor consume y destruye uploads, calcula checksums y declara residencia
inmediata: esta medida CPU incluye transcode/rig/checksum, pero ninguna espera GPU.
Las medidas finales se repitieron tras la búsqueda de nombres contados;
runtime/carga previos quedan conservados como controles, con hashes separados.

## CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.28)
project(residency_probe LANGUAGES CXX)
find_package(fastgltf CONFIG REQUIRED)
find_package(Ktx CONFIG REQUIRED)
find_package(meshoptimizer CONFIG REQUIRED)
find_package(mikktspace CONFIG REQUIRED)
foreach(version IN ITEMS before after)
 foreach(mode IN ITEMS normal memory)
  add_executable(${version}_${mode} probe.cpp)
  target_compile_features(${version}_${mode} PRIVATE cxx_std_23)
  target_compile_options(${version}_${mode} PRIVATE /GR- /EHs-c- /utf-8 /wd4530)
  if(version STREQUAL before)
   target_include_directories(${version}_${mode} PRIVATE "D:/Projects/Gloom/.cache/hito118/before/include" "D:/Projects/Gloom/include")
   target_link_libraries(${version}_${mode} PRIVATE "D:/Projects/Gloom/.cache/hito118/before-gloom_asset_pipeline.lib" "D:/Projects/Gloom/.cache/hito118/before-gloom_engine.lib")
  else()
   target_include_directories(${version}_${mode} PRIVATE "D:/Projects/Gloom/include")
   target_link_libraries(${version}_${mode} PRIVATE "D:/Projects/Gloom/build/windows-vs/Release/gloom_asset_pipeline.lib" "D:/Projects/Gloom/build/windows-vs/Release/gloom_engine.lib")
  endif()
  target_link_libraries(${version}_${mode} PRIVATE fastgltf::fastgltf KTX::ktx meshoptimizer::meshoptimizer mikktspace::mikktspace)
  if(mode STREQUAL memory)
   target_sources(${version}_${mode} PRIVATE new.cpp)
   target_compile_definitions(${version}_${mode} PRIVATE MEMORY=1)
  endif()
 endforeach()
endforeach()

```

## probe.cpp

```cpp
#include <gloom/assets/residency_coordinator.hpp>
#include <gloom/assets/scene_catalog.hpp>
#include <gloom/core/clock.hpp>
#include <stdio.h>
#include <stdlib.h>
using namespace gloom;
using namespace gloom::assets;
#ifdef MEMORY
extern volatile LONG64 measured_calls, measured_bytes, live_bytes, peak_bytes, measure_enabled;
LONG64 baseline;
void start_memory() {
    baseline = InterlockedCompareExchange64(&live_bytes, 0, 0);
    InterlockedExchange64(&measured_calls, 0); InterlockedExchange64(&measured_bytes, 0);
    InterlockedExchange64(&peak_bytes, baseline); InterlockedExchange64(&measure_enabled, 1);
}
void end_memory(uint32 stage) {
    InterlockedExchange64(&measure_enabled, 0);
    printf("stage=%u calls=%lld bytes=%lld peak=%lld delta=%lld\n", stage, measured_calls, measured_bytes,
        peak_bytes - baseline, live_bytes - baseline);
}
#else
void start_memory() {}
void end_memory(uint32) {}
#endif
void check(bool condition, const char* message) { if (!condition) { fputs(message,stderr); fputc('\n',stderr); exit(1); } }
struct Sink {
    Array<render::RenderAssetId> ids;
    uint64 checksum{0}, vertices{0}, indices{0}, texture_bytes{0};
    static bool compression(const void*) { return true; }
    static void mesh(void* context, render::MeshUpload upload) {
        Sink& self = *static_cast<Sink*>(context);
        self.ids.push_back(upload.id);
        self.checksum ^= fingerprint({reinterpret_cast<const decltype(CookedAsset{}.payload)::value_type*>(upload.vertices.data()), upload.vertices.size() * sizeof(upload.vertices[0])});
        self.checksum ^= fingerprint({reinterpret_cast<const decltype(CookedAsset{}.payload)::value_type*>(upload.indices.data()), upload.indices.size() * sizeof(upload.indices[0])});
        self.checksum ^= upload.id.value;
        self.vertices += upload.vertices.size(); self.indices += upload.indices.size();
    }
    static void texture(void* context, render::TextureUpload upload) {
        Sink& self = *static_cast<Sink*>(context); self.ids.push_back(upload.id); self.checksum ^= upload.id.value;
        for (const decltype(upload.mip_levels)::value_type& mip : upload.mip_levels) { self.texture_bytes += mip.data.size(); self.checksum ^= fingerprint(mip.data); }
    }
    static void material(void* context, render::MaterialUpload upload) { Sink& self = *static_cast<Sink*>(context); self.ids.push_back(upload.id); self.checksum ^= upload.id.value; }
    static void release(void* context, render::RenderAssetId id) {
        Sink& self = *static_cast<Sink*>(context);
        for (size_t index=0;index<self.ids.size();++index) if (self.ids[index]==id) {
            self.ids[index]=self.ids[self.ids.size()-1];self.ids.resize(self.ids.size()-1);break;
        }
    }
    static render::GpuAssetState state(const void* context, render::RenderAssetId id) {
        const Sink& self = *static_cast<const Sink*>(context);
        for (render::RenderAssetId entry : self.ids) if(entry==id)return render::GpuAssetState::resident;
        return render::GpuAssetState::missing;
    }
    AssetUploadSink view() {return {.context=this,.texture_compression_bc=compression,.mesh=mesh,.texture=texture,.material=material,.release=release,.state=state};}
};
void drain(core::JobSystem& jobs) {
    while (jobs.metrics().scheduled_jobs != jobs.metrics().completed_jobs) SwitchToThread();
}
void ready(AssetResidencyCoordinator& coordinator, SceneTicket ticket, core::JobSystem& jobs) {
    const uint64 deadline=GetTickCount64()+30000;
    do {
        coordinator.update(); check(coordinator.state(ticket)!=SceneResidencyState::failed,"Scene failed");
        if(coordinator.state(ticket)==SceneResidencyState::ready) {drain(jobs);return;}
        SwitchToThread();
    }while(GetTickCount64()<deadline);
    check(false,"Scene timed out");
}
void elapsed(uint32 stage,uint64 start,uint32 steps=1) {
    printf("stage=%u ns=%.3f\n",stage,(performance_clock()-start)*1e9/performance_frequency()/steps);
}
int main(int argc,char**argv) {
    check(argc==3,"Require cooked root and source URI");
    VirtualFileSystem filesystem;check(filesystem.mount("cache",argv[1]).has_value(),"Mount cache");
    const VirtualPath source=*VirtualPath::parse(argv[2]), cooked=*VirtualPath::parse("cache:/scene.gasset");
    decltype(discover_cooked_scene(filesystem,source,cooked)) discovered=discover_cooked_scene(filesystem,source,cooked);
    check(discovered.has_value(),"Discover cooked scene");
    core::JobSystem jobs{{.worker_threads=4,.queue_capacity=256}};check(jobs.start()==nullptr,"Jobs start");
    Sink sink;
    {
        AsyncAssetLoader loader{jobs,filesystem,discovered->catalog};
        AssetResidencyCoordinator coordinator{jobs,loader,discovered->catalog,sink.view()};
        start_memory();uint64 start=performance_clock();
        const SceneTicket ticket=coordinator.request_scene(discovered->scene);ready(coordinator,ticket,jobs);
        elapsed(0,start);end_memory(0);
        printf("geometry checksum=%llu vertices=%llu indices=%llu textures=%llu instances=%zu\n",sink.checksum,sink.vertices,sink.indices,sink.texture_bytes,coordinator.scene(ticket)->instances.size());
        start_memory();start=performance_clock();for(uint32 index=0;index<1000;++index)coordinator.update();elapsed(1,start,1000);end_memory(1);
        start_memory();start=performance_clock();coordinator.cancel(ticket);drain(jobs);elapsed(2,start);end_memory(2);
        check(sink.ids.size()==0,"Cancel retained resources");
        sink.checksum=sink.vertices=sink.indices=sink.texture_bytes=0;
        start_memory();start=performance_clock();coordinator.reload(ticket);ready(coordinator,ticket,jobs);elapsed(3,start);end_memory(3);
        printf("reload checksum=%llu vertices=%llu indices=%llu textures=%llu instances=%zu\n",sink.checksum,sink.vertices,sink.indices,sink.texture_bytes,coordinator.scene(ticket)->instances.size());
    }
    {
        AsyncAssetLoader loader{jobs,filesystem,discovered->catalog};
        AssetResidencyCoordinator coordinator{jobs,loader,discovered->catalog,sink.view(),{.new_scene_requests_per_update=0}};
        for(uint32 index=0;index<64;++index)static_cast<void>(coordinator.request_scene(discovered->scene));
        coordinator.update();start_memory();uint64 start=performance_clock();
        for(uint32 index=0;index<1000;++index)coordinator.update();elapsed(4,start,1000);end_memory(4);
    }
    jobs.stop();
}

```

## new.cpp

```cpp
#include <gloom/core/types.hpp>
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>
volatile LONG64 measured_calls=0,measured_bytes=0,live_bytes=0,peak_bytes=0,measure_enabled=0;
namespace {struct alignas(16) Header {size_t bytes;};}
void* operator new(size_t bytes) {
    Header* memory=static_cast<Header*>(malloc(sizeof(Header)+bytes));memory->bytes=bytes;
    const LONG64 live=InterlockedAdd64(&live_bytes,static_cast<LONG64>(bytes));
    if(InterlockedCompareExchange64(&measure_enabled,0,0)) {
        InterlockedIncrement64(&measured_calls);InterlockedAdd64(&measured_bytes,static_cast<LONG64>(bytes));
        LONG64 peak=InterlockedCompareExchange64(&peak_bytes,0,0);
        while(live>peak) {const LONG64 previous=InterlockedCompareExchange64(&peak_bytes,live,peak);if(previous==peak)break;peak=previous;}
    }
    return memory+1;
}
void* operator new[](size_t bytes) {return operator new(bytes);}
void operator delete(void* address) noexcept {if(!address)return;Header* memory=static_cast<Header*>(address)-1;InterlockedAdd64(&live_bytes,-static_cast<LONG64>(memory->bytes));free(memory);}
void operator delete[](void* address) noexcept {operator delete(address);}
void operator delete(void* address,size_t) noexcept {operator delete(address);}
void operator delete[](void* address,size_t) noexcept {operator delete(address);}

```

## measure.py

```python
from pathlib import Path
import ctypes, hashlib, json, os, re, subprocess, time
ROOT=Path('D:/Projects/Gloom'); HERE=ROOT/'.cache/hito118'
env=os.environ.copy(); env['PATH']=str(ROOT/'build/windows-vs/Release')+os.pathsep+env['PATH']
cases=[('small','game:/tests/material_surface/scene.gltf'),('factory','game:/legacy/factory.gltf'),('hound','game:/h07-v17/hound.gltf')]
rows=[]
for mode in ('normal','memory'):
 for name,source in cases:
  for run in range(5 if mode=='normal' else 1):
   for version in ('before','after'):
    exe=HERE/f'build/Release/{version}_{mode}.exe'
    result=subprocess.run(['rtk','proxy',str(exe),str(ROOT/f'.cache/hito117/closure/before/{name}'),source],env=env,cwd=ROOT,text=True,capture_output=True,timeout=120)
    assert result.returncode==0,(version,mode,name,result.returncode,result.stdout,result.stderr)
    rows.append(dict(version=version,mode=mode,case=name,run=run,exe_sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),output=result.stdout))
    (HERE/'load.json').write_text(json.dumps(rows,indent=2)+'\n')
   a,b=rows[-2:]
   for prefix in ('geometry','reload'):
    left=next(line for line in a['output'].splitlines() if line.startswith(prefix))
    right=next(line for line in b['output'].splitlines() if line.startswith(prefix))
    assert left==right,(name,prefix,left,right)
  print(mode,name,'complete',flush=True)
print('All geometry/mip checksums match')

```

## measure_rss.py

```python
from pathlib import Path
import ctypes, hashlib, json, os, subprocess, time
from ctypes import wintypes
ROOT=Path('D:/Projects/Gloom'); HERE=ROOT/'.cache/hito118'
env=os.environ.copy(); env['PATH']=str(ROOT/'build/windows-vs/Release')+os.pathsep+env['PATH']
class Counters(ctypes.Structure):
 _fields_=[('cb',wintypes.DWORD),('faults',wintypes.DWORD)]+[(name,ctypes.c_size_t) for name in
  ('peak_working_set','working_set','quota_peak_paged','quota_paged','quota_peak_nonpaged','quota_nonpaged','pagefile','peak_pagefile','private')]
query=ctypes.windll.psapi.GetProcessMemoryInfo
query.argtypes=[wintypes.HANDLE,ctypes.POINTER(Counters),wintypes.DWORD]
rows=[]
for name,source in [('small','game:/tests/material_surface/scene.gltf'),('factory','game:/legacy/factory.gltf'),('hound','game:/h07-v17/hound.gltf')]:
 for version in ('before','after'):
  exe=HERE/f'build/Release/{version}_normal.exe'
  # The outer shell uses RTK; measuring the native child excludes the wrapper's memory.
  with (HERE/f'rss-{version}-{name}.log').open('w',encoding='utf-8') as log:
   proc=subprocess.Popen([str(exe),str(ROOT/f'.cache/hito117/closure/before/{name}'),source],env=env,cwd=ROOT,stdout=log,stderr=subprocess.STDOUT)
   peak=private=0
   while proc.poll() is None:
    counters=Counters(); counters.cb=ctypes.sizeof(counters)
    if query(wintypes.HANDLE(int(proc._handle)),ctypes.byref(counters),counters.cb):
     peak=max(peak,counters.peak_working_set); private=max(private,counters.private)
    time.sleep(.002)
  assert proc.returncode==0,(name,version,proc.returncode)
  rows.append(dict(case=name,version=version,peak_working_set=peak,sampled_peak_private=private,exe_sha256=hashlib.sha256(exe.read_bytes()).hexdigest()))
  (HERE/'rss.json').write_text(json.dumps(rows,indent=2)+'\n')
  print(name,version,'peak WS MiB',peak/1048576,'private MiB',private/1048576,flush=True)

```
