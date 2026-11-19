"""第15步临时本机UDP限速器：原样转发真实游戏包，按UDP/IP开销计费，不生成协议内容。"""
import collections,json,select,socket,struct,sys,time
front=socket.socket(socket.AF_INET,socket.SOCK_DGRAM);front.bind(('127.0.0.1',27320));front.setblocking(False)
back=socket.socket(socket.AF_INET,socket.SOCK_DGRAM);back.connect(('127.0.0.1',27090));back.setblocking(False)
control=socket.socket();control.bind(('127.0.0.1',27321));control.listen(2);control.setblocking(False)
client=None;queue=[collections.deque(),collections.deque()];queued=[0,0]
totals={'received':[0,0],'sent':[0,0],'dropped':[0,0],'peak_queue_bytes':[0,0],'kinds':[{},{}]}
tokens=[8192.,8192.];rates=[375000,int(sys.argv[2]) if len(sys.argv)>2 else 375000];last=time.monotonic();until=0;running=True;log=open(sys.argv[1],'w',encoding='utf-8')
drop_acks=0;seen=collections.OrderedDict();totals['repeated_down_fragments']=0;totals['injected_ack_drops']=0
def state():
    return dict(totals,queued_bytes=queued,queued_packets=[len(q) for q in queue],rates=rates,slow_seconds=max(0,until-time.monotonic()),client=client)
def event(kind):
    log.write(json.dumps(dict(event=kind,time=time.time(),**state()))+'\n');log.flush()
event('start');print('ready: UDP 27320 -> 27090; control TCP 27321',flush=True)
next_report=last+1
try:
    while running:
        now=time.monotonic()
        if until and now>=until:rates[0]=375000;until=0;event('restored')
        for d in (0,1):tokens[d]=min(8192,tokens[d]+rates[d]*(now-last))
        last=now
        ready,_,_=select.select([front,back,control],[],[],.001)
        for s in ready:
            if s is control:
                c,_=control.accept();c.settimeout(1)
                try:
                    cmd=c.recv(64).decode().strip()
                    if cmd=='slow':rates[0]=62500;tokens[0]=0;until=time.monotonic()+30;event('slow')
                    elif cmd in ('drop-acks','drop-one-ack'):drop_acks=1 if cmd=='drop-one-ack' else 20;event('arm_ack_loss')
                    elif cmd=='stop':running=False
                    elif cmd!='status':raise ValueError('unknown proxy command')
                    c.sendall((json.dumps(state())+'\n').encode())
                finally:c.close()
                continue
            d=0 if s is back else 1
            for _ in range(128):
                try:data,addr=s.recvfrom(65536)
                except BlockingIOError:break
                except ConnectionResetError:break
                if d==1:
                    if addr[0]!='127.0.0.1' or (client is not None and addr!=client):continue
                    client=addr
                if not client:continue
                size=len(data)+28;totals['received'][d]+=size
                # 只读已知固定帧头以统计消息种类，不修改或解释正文。
                kinds=[]
                if data[:4]==b'ICRF' and len(data)>=64:
                    kinds=[struct.unpack_from('<H',data,48)[0]]
                    if d==0:
                        key=(struct.unpack_from('<Q',data,24)[0],struct.unpack_from('<I',data,40)[0])
                        if key in seen:totals['repeated_down_fragments']+=1
                        seen[key]=True
                        if len(seen)>8192:seen.popitem(last=False)
                elif data[:4]==b'ICRB' and len(data)>=28:
                    at=28
                    for _ in range(min(16,struct.unpack_from('<H',data,24)[0])):
                        if at+24>len(data):break
                        length,kind=struct.unpack_from('<HH',data,at);kinds.append(kind);at+=22+length
                for kind in kinds:totals['kinds'][d][str(kind)]=totals['kinds'][d].get(str(kind),0)+1
                if d==1 and drop_acks and kinds and 9 in kinds:
                    drop_acks-=1;totals['dropped'][d]+=1;totals['injected_ack_drops']+=1;event('drop_ack');continue
                # 超出8MiB或8192包时明确计丢弃，不让模拟器自身形成无界内存。
                if queued[d]+size>8*1024*1024 or len(queue[d])>=8192:totals['dropped'][d]+=1;continue
                queue[d].append(data);queued[d]+=size;totals['peak_queue_bytes'][d]=max(totals['peak_queue_bytes'][d],queued[d])
        for d in (0,1):
            while queue[d] and tokens[d]>=len(queue[d][0])+28:
                data=queue[d][0];size=len(data)+28
                try:sent=front.sendto(data,client) if d==0 else back.send(data)
                except BlockingIOError:break
                if sent!=len(data):raise RuntimeError('partial datagram')
                queue[d].popleft();queued[d]-=size;tokens[d]-=size;totals['sent'][d]+=size
        if now>=next_report:event('sample');next_report=now+1
finally:
    event('stop');front.close();back.close();control.close();log.close()
