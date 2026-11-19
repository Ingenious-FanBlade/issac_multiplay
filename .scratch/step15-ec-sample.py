"""布局 84 的双端实机读取。用累计计数差分计算当前压力窗，不混入冷建帧。"""
import json
from pathlib import Path
import socket
import sys
import time
from step15_sample import query

HOST_LOG = Path(r"F:\projects_isaac\Isaac-for-test\host\The Binding of Isaac Rebirth\isaac-dual-profile\Documents\My Games\Binding of Isaac Repentance+\log.txt")
def proxy(action):
    with socket.create_connection(("127.0.0.1", 27111), timeout=3) as connection:
        connection.sendall((action+"\n").encode())
        return json.loads(connection.makefile("rb").readline())

def capture():
    client = query(27092, "cs stream client 0") | query(27092, "cs stream client 1")
    state = query(27092, "cs status")
    host = query(27091, "cs stream source 0") | query(27091, "cs stream main 37")
    return dict(time=time.perf_counter(),client=client,state=state,host=host,proxy=proxy("status"))

def diff(earlier, later, key):
    return int(later.get(key,0))-int(earlier.get(key,0))

def report(start, end, loads):
    dt=end["time"]-start["time"]
    a,b=start["client"],end["client"]
    result={}
    for stream in ("world","body"):
        prefix=f"stream.binary.{stream}."
        stages={}
        for name in ("receive","decode_slice","plan_slice","ack","install","publish","end_to_end","manager"):
            key=prefix+name
            n=diff(a,b,key+".n")
            if n:
                stages[name]=dict(n=n,hz=round(n/dt,3),mean_us=round(diff(a,b,key+".sum_us")/n,2),
                    over_2ms=diff(a,b,key+".over_2000_us"),over_4ms=diff(a,b,key+".over_4000_us"))
        result[stream]=dict(stages=stages,
            waits={k[len(prefix+"wait."):]:diff(a,b,k) for k in b if k.startswith(prefix+"wait.") and diff(a,b,k)})
    source = "stream.main.duration.ordinary_published_outer."
    ordinary_samples = diff(start["host"],end["host"],source+"wall_count")
    ordinary_us = diff(start["host"],end["host"],source+"wall_total_us")
    return dict(seconds=dt,client=result,host_ordinary_timing=dict(
        n=ordinary_samples,mean_us=round(ordinary_us/ordinary_samples,2) if ordinary_samples else None),
        client_state={k:end["state"].get(k) for k in ("client_state","client.error","client.scene_state","client.applied_revision","client.native_reason")},
        host={k:diff(start["host"],end["host"],k) for k in end["host"] if k.startswith("stream.source.") and k.endswith((".ordinary",".bodies",".gaps",".unchanged.roots",".unchanged.checks",".root_resource_checks",".root_resource_revocations",".object_failures"))},
        proxy={k:end["proxy"][k]-start["proxy"][k] for k in ("world","dropped","delayed","down_bytes","up_bytes")},
        load=dict(samples=len(loads),steady=sum(x.get("static_tears")==300 and x.get("tears")==300 and x.get("participants")==2 and len(x.get("bodies",[]))==4 for x in loads),
            examples=loads[:1]+loads[-1:]))

def sample(seconds, mode):
    proxy(mode)
    # 等线路进入目标模式、清除前一模式的短时尾部后，记录同一时间窗口的两端计数。
    time.sleep(.2)
    with HOST_LOG.open("r",encoding="utf-8",errors="replace") as log:
        log.seek(0,2)
        a=capture()
        ticks=[]
        deadline=time.monotonic()+seconds
        while time.monotonic()<deadline:
            time.sleep(min(5,max(0,deadline-time.monotonic())))
            state=query(27092,"cs status")
            ticks.append({k:state.get(k) for k in ("client_state","client.error","client.scene_state","client.applied_revision")})
            if state.get("client_state") in ("Off","Failed"):
                break
        b=capture()
        loads=[]
        for line in log:
            marker="[isaac-cs-test] tx21 "
            if marker not in line: continue
            try:
                entry=json.loads(line.split(marker,1)[1])
                if entry.get("event")=="load": loads.append({k:entry.get(k) for k in ("static_tears","tears","participants","effects","bodies","room_frame")})
            except json.JSONDecodeError:
                continue
    result=report(a,b,loads)
    result["client_samples"]=ticks
    Path(sys.argv[3]).write_text(json.dumps(dict(before=a,after=b,summary=result),ensure_ascii=False,indent=2),encoding="utf-8")
    print(json.dumps(result,ensure_ascii=False,indent=2),flush=True)

if __name__=="__main__":
    sample(float(sys.argv[1]),sys.argv[2])
