# EntityPtr

由唯一布局源生成。布局编号 10；布局版本 44；原生大小 0x4；未知 0B；完整：是。

EntityPtr::SetReference 会维护 Entity 内引用登记链；网络只解析稳定成员索引，不能直接复制指针。

原生字节归属：

| 0x0 Entity* entity【引用】实体智能引用；只发送成员表内实体序号，不发送地址（4B；已核） |
|---|

状态二进制布局（总计 2—2B，不含第 9 步记录头）：

块 0：实体引用（body，2—2B）：

| +0x0 u16 entity：实体智能引用；只发送成员表内实体序号，不发送地址 |
|---|

证据：

- `entity`：J273 VA 0x858760 EntityPtr::SetReference（RVA 0x458760，imagebase 0x400000）；官方 EntityPtr.zhl
