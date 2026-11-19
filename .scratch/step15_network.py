"""真实双端限速与恢复窗口。独立记录代理字节、游戏发布、CPU和有界资源。"""
import json,socket,sys,time
from pathlib import Path
from step15_perf import snapshot,summary,brief
def proxy(command):
    with socket.create_connection(('127.0.0.1',27101),timeout=3) as s:
        s.sendall(command.encode());return json.loads(s.makefile('rb').readline())
out=[]
for label,duration in [('baseline',60),('slow',30),('recovery',60)]:
    before=snapshot();p0=proxy('slow' if label=='slow' else 'status');deadline=time.monotonic()+duration
    while time.monotonic()<deadline:
        time.sleep(min(10,max(0,deadline-time.monotonic())))
        state=brief(None);print(label,json.dumps(state),flush=True)
        if state.get('client_state')=='Off':break
    after=snapshot();p1=proxy('status');item={'phase':label,'before':before,'after':after,'proxy_before':p0,'proxy_after':p1,'summary':summary(before,after)};out.append(item)
    Path(sys.argv[1]).write_text(json.dumps(out,indent=2),encoding='utf-8')
    print(label,json.dumps(item['summary']['rates']),json.dumps(p1),flush=True)
    if state.get('client_state')=='Off':break
