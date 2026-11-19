"""第15步停下时的最小转储读取器，只读异常、模块与捕获内存。"""
import struct
import sys

class Dump:
    def __init__(self, path):
        self.data = open(path, 'rb').read()
        self.streams = {}
        count, directory = struct.unpack_from('<II', self.data, 8)
        for i in range(count):
            kind, size, rva = struct.unpack_from('<III', self.data, directory+i*12)
            self.streams[kind] = (rva, size)
        self.ranges = []
        if 5 in self.streams:
            rva, _ = self.streams[5]
            for i in range(self.u32(rva)):
                base, size, file = struct.unpack_from('<QII', self.data, rva+4+i*16)
                self.ranges.append((base, base+size, file))
        if 9 in self.streams:
            rva, _ = self.streams[9]
            count, file = struct.unpack_from('<QQ', self.data, rva)
            for i in range(count):
                base, size = struct.unpack_from('<QQ', self.data, rva+16+i*16)
                self.ranges.append((base, base+size, file))
                file += size
        self.modules = []
        rva, _ = self.streams[4]
        for i in range(self.u32(rva)):
            entry = rva+4+i*108
            base, size = struct.unpack_from('<QI', self.data, entry)
            name_rva = self.u32(entry+20)
            name = self.data[name_rva+4:name_rva+4+self.u32(name_rva)].decode('utf-16-le')
            self.modules.append((base,base+size,name))
        rva, _ = self.streams[6]
        self.code = self.u32(rva+8)
        self.address = struct.unpack_from('<Q',self.data,rva+24)[0]
        self.parameters = struct.unpack_from('<15Q',self.data,rva+40)[:self.u32(rva+32)]
        context = self.u32(rva+164)
        names = ('edi','esi','ebx','edx','ecx','eax','ebp','eip','cs','eflags','esp','ss')
        self.regs = dict(zip(names,struct.unpack_from('<12I',self.data,context+156)))
    def u32(self, offset):
        return struct.unpack_from('<I',self.data,offset)[0]
    def read(self, address, size):
        for start,end,file in self.ranges:
            if start<=address and address+size<=end:
                return self.data[file+address-start:file+address-start+size]
        raise ValueError('memory not captured '+hex(address))
    def symbol(self, address):
        for start,end,name in self.modules:
            if start<=address<end:
                return name.rsplit('\\',1)[-1]+'+'+hex(address-start)
        return hex(address)

if __name__=='__main__':
    d=Dump(sys.argv[1])
    print('exception',hex(d.code),d.symbol(d.address),[hex(x) for x in d.parameters])
    print({n:hex(v) for n,v in d.regs.items()})
    esp=d.regs['esp']
    for i in range(128):
        try: value=struct.unpack('<I',d.read(esp+i*4,4))[0]
        except ValueError: break
        name=d.symbol(value)
        if '+' in name: print(hex(esp+i*4),name)
