# ST5 agent artifact 说明

所有 VA 均对应 IDB imagebase 0x400000 的静态地址；RVA 按 VA - 0x400000 记录。目标输入名为 isaac-ng-v1.9.7.12.exe.unpacked.exe。查询均为 IDA 只读 py_eval、decompile、disasm 或 analyze_batch；未调用数据库写入 API，没有修改 IDB。

- rtti-col-chd-vtables.json：一次 py_eval 直接读 16 个目标 vtable 的 slot 0/1、vtable[-1] COL、COL words、TypeDescriptor 名称、CHD 和 BCD words；附 IDB 输入路径、imagebase、segment bounds。覆盖七个 Entity 类、Entity、GridEntity、Door 及六个抽查到的其他 Entity 派生类。
- free-wrapper-analysis.json：analyze_batch 对七类/基类/ Grid/Door slot 0 wrapper 与六个抽查到的其他 Entity 类 slot 0 进行 decompile、disasm、常量/块分析。
- factory-decompile.json：mcp__ida_mcp__decompile 对 0x6EC540 获取完整带地址伪代码及 refs；附重点地址行和函数大小/调用者分析元信息。
- derived-dtor-evidence.json：Entity 基析构和五个独立派生析构反编译，以及六个派生/Free wrapper 的直接 disassembly；Player 的 call 指令为 0x74F67F，反编译行注释的地址 0x74F684 是 call 后续地址。
- 初次 segment 查询把 IDAPython Segments() 返回的地址当成 segment 对象，初次 comprehension 查询在 IDA 的本地作用域触发 NameError；修正为 getseg 和显式循环后成功。两次均只读，没有对 IDB 产生副作用。
