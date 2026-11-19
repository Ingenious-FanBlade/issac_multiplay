"""第15步实机压力窗口：只读控制口，保存临时差分与分桶，不代替真实客机。"""
import json
import sys
import time
from pathlib import Path
from step15_sample import query

def snapshot():
    """各端只使用本端累计量，频率使用编排器墙钟。"""
    result = {}
    for name, port, commands in (
        ('host', 27091, ['cs status', 'cs stream source 0', 'cs stream 1 0'] +
         [f'cs stream main {p}' for p in (12,13,15,19,20,21,22,23,24,25,26,27,30,31,32,33,34,35,36,37,38,39,40,41,42,43)]),
        ('client', 27092, ['cs status', 'cs stream client 0', 'cs stream client 1']),
    ):
        values = {}
        for command in commands:
            sample = query(port, command)
            if command.startswith('cs stream main '):
                # 每页的分桶与其本次计数配对，避免后续页覆盖分母。
                prefixes = {key.split('bucket_log2_us.')[0] for key in sample if 'bucket_log2_us.' in key and 'thread_cpu_' not in key}
                values.update({key:value for key,value in sample.items()
                    if '.duration.' not in key or any(key.startswith(prefix) for prefix in prefixes)})
            else:
                values.update(sample)
        result[name] = values
    result['time'] = time.perf_counter()
    return result

def brief(value):
    """周期输出实际发布、应用、错误及有界资源，避免逐字段日志污染窗口。"""
    h = query(27091, 'cs stream source 0')
    c = query(27092, 'cs status')
    return {k: v for k, v in (h | c).items() if k in (
        'stream.source.ordinary','stream.source.gaps','stream.source.reason',
        'stream.source.capture_detail','stream.source.member_indices.high_water',
        'stream.source.member_indices.reused','client.applied_revision',
        'client.scene_state','client.error','client_state')}

def summary(start, end):
    dt = end['time'] - start['time']
    out = {'seconds': dt, 'rates': {}, 'timings': {}, 'final': {}}
    for side in ('host','client'):
        a,b = start[side], end[side]
        for key,value in b.items():
            if key.startswith('stream.binary.') and key.endswith('.sum_us'):
                prefix=key[:-len('sum_us')]
                n=int(b.get(prefix+'n',0))-int(a.get(prefix+'n',0))
                if n>0:
                    timing={'n':n,'mean_us':(int(value)-int(a.get(key,0)))/n}
                    # 只有Install/Manager/端到端导出阈值计数；其余缺失不能伪报为零。
                    for threshold in ('over_2000_us','over_4000_us'):
                        if prefix+threshold in a and prefix+threshold in b:
                            timing[threshold]=int(b[prefix+threshold])-int(a[prefix+threshold])
                    out['timings'][side+'.'+prefix]=timing
            if key.endswith('wall_count'):
                prefix = key[:-len('wall_count')]
                n = int(value)-int(a.get(key,0))
                if n <= 0: continue
                buckets = [(i, int(b.get(prefix+f'bucket_log2_us.{i}',0))-int(a.get(prefix+f'bucket_log2_us.{i}',0))) for i in range(32)]
                quantiles = {}
                if sum(v for _,v in buckets) == n:
                    for percentile in (50,95,99):
                        seen=0
                        for i,count in buckets:
                            seen+=count
                            if seen >= (n*percentile+99)//100:
                                quantiles[f'p{percentile}_upper_us']=(2<<i)-1
                                break
                out['timings'][side+'.'+prefix]={'n':n,'mean_us':(int(b.get(prefix+'wall_total_us',0))-int(a.get(prefix+'wall_total_us',0)))/n,**quantiles}
                for threshold in ('over_2000_us','over_4000_us'):
                    if prefix+threshold in a and prefix+threshold in b:
                        out['timings'][side+'.'+prefix][threshold]=int(b[prefix+threshold])-int(a[prefix+threshold])
            if key.endswith('.publish.n') or key in ('completed_tick','stream.source.ordinary','stream.source.bodies','stream.wire_bytes'):
                out['rates'][side+'.'+key]=(int(value)-int(a.get(key,0)))/dt
        out['final'][side]={k:v for k,v in b.items() if k in ('state','last_error','client_state','client.error','client.scene_state','stream.source.reason','stream.source.gaps','stream.source.capture_detail','stream.first_failure','stream.source.member_indices.high_water','stream.source.member_indices.reused') or k.startswith('stream.pool.')}
    return out

if __name__ == '__main__':
    duration=float(sys.argv[1]); path=Path(sys.argv[2])
    first=snapshot(); deadline=time.perf_counter()+duration
    while time.perf_counter()<deadline:
        time.sleep(min(10,max(0,deadline-time.perf_counter())))
        current=brief(None)
        print(json.dumps(current,ensure_ascii=False),flush=True)
        if current.get('client_state')=='Off': break
    last=snapshot(); result=summary(first,last)
    path.write_text(json.dumps({'before':first,'after':last,'summary':result},ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps(result,ensure_ascii=False,indent=2),flush=True)
