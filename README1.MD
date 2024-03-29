### 一、什么是 sisdb ?

---

sisdb是一个完全开源的使用ANSI C语言编写、支持本地文件、网络、内存、数据可持久化的时序数据库；

主要应用于有价证券市场行情采集、因子数据存取、主要优势在处理大吞吐量的时序结构化数据；

主要具备部署简单、低延迟、查询数据格式丰富、多数据表流式回放等特点；

### 二、为什么需要 sisdb

---

1. 在现有的数据库中，主流的关系数据库因为吞吐量的原因无法满足高速大量的数据写入，即便是现在最流行的mongodb，当发生随机写入时效率也会大幅度降低；而作为吞吐量足够的内存数据库redis，但是其所提供的数据结构却并未针对有价证券体系，和普通的文件存储解决方案相比并没有什么优势；
2. 生产环境和开发环境对数据格式要求的不同，对于开发者来说最好的情况是开发阶段用json格式做数据交互；进入生产环境使用压缩（且加密）的二进制数据流；这样即保证了开发的效率，同时可以平滑切换到安全可靠的生产环境；
3. 越来越多的跨平台开发需求，使得目前跨系统的前端通常以WEB应用为主；sisdb直接提供了websocket标准通讯协议来提供数据支持，使得针对跨平台的客户端数据支持有先天的优势；
4. sisdb 是专门为有价证券体系开发的一款高性能、安全、便捷的时序结构化数据库；sisdb针对有价证券的行情和交易数据进行优化存取，不仅支持二进制结构化数据流，而且还支持 json、array、csv 数据格式的输入和输出，便于用户分析数据；
5. sisdb提供插件式功能扩展，便于二次开发，很方便的扩展功能，并且支持python的插件开发；

### 三、sisdb 的特性

---

#### 3.1 输入和输出【速度】瓶颈仅限于网络带宽;

查询应保证绝大部分都在内存中进行, 除非特定指出,否则只会读取和处理内存中的数据;

写入应保证为硬盘顺序写入的速度, 根据限定内存大小自动存储非活跃数据至存储设备;

由于默认只有时间为索引, 因此建立索引的时间基本可以忽略不计, 并能快速定位;

数据安全性和完整性保证是依据以下流程:

收到写入指令首先写入顺序文件log中,成功后再处理到数据库中, 当掉电等异常出现时, 首先加载磁盘文件再处理log中的顺序指令, 以恢复掉电前数据库的状态, 最后再对外提供服务;

#### 输入和输出【格式】支持二进制数据流和字符串数据流;

二进制数据流包括结构化数据、压缩数据格式传输;

字符串数据流包括JSON格式、数组格式、CSV格式的输入和输出, 对JSON格式还应该支持变量字段传输, 其他字段自动增补的功能;

可以指定字段输出、指定时间段输出、指定数量输出;

数据库的磁盘文件也可以设定格式为二进制和字符串文件格式，方便数据库数据分析、备份和迁移;

#### 输入一个时间节点的价格,数据库可以自动完成不同时间尺度的数据聚合,而不需要操作者去控制;

这里引入了数据层级的概念, 数据层级的引入,极大的减轻了数据采集者的工作量,同时提高了访问者的速度和便捷性；

数据层级主要表现在 细粒度的时间数据可以分发给粗粒度的时间数据, 并且这些分发和数据合并并不需要人为干预，全部由系统自动完成；

#### 输入和输出的【通讯协议】支持websocket协议;

以浏览器为基础的客户端会成为跨平台的主流，因此sisdb支持websocket来提供数据支持;

客户可以直接使用 sisdb + sischart 来进行图表展示和策略分析;

=======

> 在现有的数据库中，主流的关系数据库因为吞吐量的原因无法满足高速大量的数据写入，Nosql数据库的代表mongodb，当发生随机写入时效率也会大幅度降低；内存数据库的代表虽然吞吐量满足需求，但他更适合做一个通讯中间件，不能支持超过内存大小数据的管理；再说一个Elasticsearch其主要应用方向是日志分析；

> 生产环境和开发环境对数据格式要求的不同，对于开发者来说最好的情况是开发阶段用json格式(或其他字符格式)做数据交互，并且数据库存储格式也可以指定为json；一旦进入生产环境就使用压缩（且加密）的二进制数据流，提高系统性能和安全性；

> 越来越多的跨平台开发需求，要求对各种语言，各种操作系统最好都能支持，sisdb完全支持linux、windows、macos三种操作系统编译，网络协议采用websocket标准通讯协议，交互协议采用KV可拆装协议、使用者学习曲线平滑、适配性强；

> sisdb 最初是专门为有价证券开发的一款高性能、安全、便捷的证券行情数据库；sisdb针对有价证券的行情和交易数据进行优化存取，不仅支持二进制结构化数据流，而且还支持 json、array、csv 数据格式的输入和输出，通过python客户端接口可以方便的获取需要的格式；

#### 集群节点支持(V2.0);

V2.0中会支持集群功能,并对所有节点进行分层,这里每个节点并不会对数据进行分片, 而是保留了全部的数据, 扩容时对节点完整复制,执行初始化指令就加入到集群中了; 这样的话即便系统中99%的节点不可恢复的崩溃, 只要有一个节点, 其他节点即可恢复;

这样设计的目的是因为证券策略的私密、高速、运算密集的特性决定的, 具有独立而灵活的特点;

根节点可能只有最基本的数据, 而子节点可以复制上级节点的所有数据,

### sisdb 的设计原理

---

#### 分为以下3种格式数据表:

=======

### 输入和输出【格式】支持二进制数据流和字符串数据流;

    二进制数据流包括结构化数据、压缩数据格式传输;
       字符串数据流包括JSON格式、数组格式、CSV格式的输入和输出, 对JSON格式还应该支持变量字段传输, 其他字段自动增补的功能;
       可以指定字段输出、指定时间段输出、指定数量输出;

### 输入一个时间节点的价格,数据库可以自动完成不同时间尺度的数据聚合,而不需要操作者去控制;

    基于时间尺度的数据层级, 可以减少网络的数据流量、减轻了数据采集者的工作量, 同时提高了访问者的速度和便捷性；
    数据层级主要表现在 细粒度的时间数据可以server端分发给粗粒度的时间数据, client只需要写入细粒度数据，就可以自动生成各种时间尺度的数据；

### 输入和输出的【通讯协议】支持 websocket 协议;

    以浏览器为基础的客户端会成为跨平台的主流，因此 sisdb 支持 websocket 来提供数据支持;
       客户可以直接使用 sisdb + sischart 来进行图表展示和策略分析;

### 集群节点支持(V2.0);

    V2.0中会支持集群功能,并对所有节点进行分层,这里每个节点并不会对数据进行分片, 而是保留了全部的数据, 扩容时对节点完整复制,执行初始化指令就加入到集群中了; 这样的话即便系统中99%的节点不可恢复的崩溃, 只要有一个节点, 其他节点即可恢复;
       这样设计的目的是因为证券策略的私密、高速、运算密集的特性决定的, 具有独立而灵活的特点;
       根节点可能只有最基本的数据, 而子节点可以复制上级节点的所有数据,跟节点并不会同步子节点数据，只会同步同等级和上一级的数据；

1、memdb

存储单键值数据，数据类型可以是字符串，数据流，列表等；
======================================================

### 以下4种格式数据表:

    1、memdb
         存储键值数据，
         订阅和发布数据，
         全部数据都在内存，记录网络log，便于历史数据备份回放；
      2、sisdb
         主要存储结构化数据，数据类型为结构化数据，日期型的数据按年储存，其他的数据按天存储，数据支持增删插；
         支持单条任意格式数据、支持列表任意格式数据；
         按用户读取热度加载部分数据到内存；
         这种数据内存加载方式的好处是，可以避免数据写入后没有读者，而浪费内存资源；
      3、snodb
         仅存储结构化数据，数据类型为结构化数据，按天存储到不同目录下，数据只支持增量写入模式；
         内存仅保留当日的数据，当日数据结束后就转存到磁盘中；
      4、netdb
         用户网络压缩传输格式，数据类型为结构化数据，增量写入模式；
         内存仅保留当日的数据，当日数据结束后就转存到磁盘中；

### 内存和磁盘数据的同步:

    针对sisdb数据结构对磁盘数据建立内存映射表、按读取热度确定是否保留在内存；和mmap不同之处在于，采用读优先策略管理内存；

全部加载到内存；

2、sisdb

存储结构化数据，数据类型为结构化数据，日期型的数据按年储存，其他的数据按天存储，数据支持增删插；

按用户需求加载部分数据到内存；
==============================

   1、下载并安装sisdb

3、sdcdb

存储结构化数据，数据类型为流式结构化数据，按天存储到不同目录下，数据只支持增量写入模式；

内存仅保留当日的数据，当日数据结束后就转存到磁盘中；

### 开始使用 sisdb

---

#### 下载安装

以redis插件形式安装

1、下载并安装sisdb

```shell
    git clone  https://github.com/seerline/sisdb.git
    cd sisdb/bin
    make
    ./sisdb -f sisdb-server.conf 
```

程序安装完成；
==============

```
   程序安装和启动完成；

2、运行客户端开始体验

      在浏览器中打开 sisdb/client/web/index.html

`show`列出所有数据集合的名称



### 指令集
----

```

> [db].show  [key]

```
   列出[db]数据集合中的所有数据表和基本信息
```

> [db].get [key].day [command]

```
   [db]  表示从哪个数据集合分发的指令
   [key] 表示取哪只股票数据

   [command] 可有可无，由command定义数据范围和字段范围

   command为json命令
   `stsdb.get collects.exch {"format":"json"}`

以json格式返回exch的所有记录名；
   获取exch库的所有市场号

=======
   sisdb.get SH600600.info {"format":"json"}
   以json格式返回 SH600600.info 的所有记录名；
   用户传入的command中关键字的定义如下: 

           "format": 表示格式
                  "json" --> STS_DATA_JSON  默认返回json全部数据
   				   "array" --> STS_DATA_ARRAY 仅仅返回value数组
   				   "struct" --> STS_DATA_BIN  
   				   "string" --> STS_DATA_STRING
   				   "zip" --> STS_DATA_ZIP
   字段:     "fields":  "time,close,vol,name" 表示一共4个字段  
               空---->表示全部字段
   --------<以下区域没有表示全部数据>--------
   数据范围: "range":   
                 min 为索引开始值  max 为索引结束值
                 start，stop 按记录号取数据 0，-1-->倒数第一条
   				  count (和max或stop互斥，正表示向后，负表示向前),
   				  
   数据范围:  "match" : 按字段完全匹配 匹配成功才返回数据


   返回数据为: 
    [[]];
```

> [db].set [key].day [data]

```
   以json格式传输数据 可以是{},也可以是 []

   [db]  表示从哪个数据集合分发的指令
   [key] 表示取哪只股票数据，必须有明确的代码 
   [data] 表示跟的数据，
```

> [db].out [key].day [format]

```
   以二进制格式传输数据 

   [db]  表示从哪个数据集合分发的指令
   [key] 表示取哪只股票数据，必须有明确的代码 
   [format] 表示输出数据的格式， 支持json、array、csv、二进制格式
=======


### 配置说明
----

#### 目录说明

   bin - 输出文件、配置文件等

   src - 源码

   api - c 语言的数据库调用接口

   client - 客户端访问数据库的例子

   test - 单元测试

   data - 默认数据文件存放位置（运行期目录）

   logs - 日志（运行期目录）

#### 主配置文件
```

sisdb.server.conf  -- 作为独立服务时使用的配置文件
主要配置文件
详见配置文件中说明

```

#### 数据结构配置说明


#### 数据结构字段说明
```

字段名 | 数据类型 | 长度 | 数量(默认为1) | 小数点(默认为0) | 是否索引(不重复字段)

```
   > 数据类型
   
      1、 'T'  毫秒        8  
      2、 'S'  秒          4、8字节  
      3、 'M'  分钟        4字节      time_t/60
      4、 'D'  天          4字节 20010101    
      5、 'I'  整数        1、2、4、8字节
      6、 'U'  无符号整数   1、2、4、8字节
      7、 'C'  字符型      1..N 
      8、 'F'  浮点数      4、8
=======
   数据结构字段说明
   | 字段名     | 数据类型 | 长度 | 数量 | 小数点 | 字段约束 | 
   | :---      | :----: | :----: | :----: | :----: | :----: |
   | 字符串     | 参考数据类型定义 | 字段长度 | 默认为1 | 默认为0 | I 为索引 O 为唯一 | 

   > 数据类型定义
   | 数据类型标志 | 数据类型名 | 字节长度 | 样例 | 
   | :---           | :----: | :----: | ----: |
   |  T   |毫秒      | 8       | 1630906080840  |
   |  S   |秒        |4、8     | 1630906080|
   |  M   |分钟      |4        | 27181768 |
   |  D   |天        |4        | 20100101|
   |  I   |整数      |1、2、4、8| -1234|
   |  U   |无符号整数 |1、2、4、8| 1234|
   |  C   |字符型     |1..N     | SH600600 |
   |  F   |浮点数     |4、8     | 1234.56|



## sisdb 的通讯协议规范

 [通讯协议](./src/core/net/readme.md)


## sisdb 的程序架构和扩展性

```plantuml

state forkstatus <<fork>>
state joinstatus <<join>>
state stopstatus <<end>>

[*] --> websocket
websocket --> server
server --> forkstatus
forkstatus --> sisdb
forkstatus --> memdb
forkstatus --> snodb
forkstatus --> ...
sisdb --> joinstatus
memdb --> joinstatus
snodb --> joinstatus
... --> joinstatus
joinstatus --> client
client --> stopstatus

```

### 集群

---

 对于集群来说, 低等级的节点数据总是以高等级节点为标准的, 这样可以统一因各种原因造成的分片数据不统一的问题, 低级节点可主动请求数据, 高级节点仅在特定时候要求下级节点进行数据更新；

 设计中对集群数据不分片，任何时候任何节点都可以从高级节点获取全部基础数据；

> (暂时不支持)
