# 编译原理课程实践报告：SysY编译器

信息科学技术学院 2200013188 李天宇

## 一、编译器概述

### 1.1 基本功能

本编译器基本具备如下功能：
1. 将`SysY`文件编译为`koopa`中间代码
2. 将`SysY`文件编译为`riscv`机器代码（`-perf`同`-riscv`）

### 1.2 主要特点

本编译器的主要特点是，在使用`parser`解析程序后，直接从`ast`树构建内存形式的`koopa_raw_program`，而非生成字符串形式的`koopa`（开局一个`koopa.h`，代码全靠猜）。

## 二、编译器设计

### 2.1 主要模块组成

- 将`SysY`解析为`ast`树的`parser`。由`sysy.l`与`sysy.y`构成
- `ast`结构。在`ast.h`中定义了`ast`的结构，在`ast.cpp`中实现了以`DFS`形式构建`koopa_raw_program`的`toRaw()`函数
- 符号表。在解析程序过程中，将各种变量、函数名称作为符号存入多级符号表，在`symtab.h`与`symtab.cpp`中实现
- 目标代码生成。在`visitraw.h`与`visitraw.cpp`中定义了一系列`visit`函数与辅助函数，用于遍历`koopa_raw_program`，生成目标代码

### 2.2 主要数据结构

本编译器中，主要实现的结构是各个语法成分所对应的的`ast`树。

我们注意到，在`koopa.h`的实现中，使用了`tagged union`这种`rust`特有写法的`C`实现，在各个`ast`结构的实现过程中，也借鉴了这种做法：在类中写一个`enum`来区分它的具体类型，然后将它的后代作为指针写在类中。但我们并未像`koopa.h`中那样，把每种类型的属性全区分开，而是共用：毕竟一棵`ast`树的类型是唯一的。

```cpp
class OpenStmtAST : public BaseAST
{
public:
    enum Type {
        IF,
        IF_ELSE,
        WHILE
    } type;

    std::unique_ptr<BaseAST> exp;
    std::unique_ptr<BaseAST> stmt;
    std::unique_ptr<BaseAST> closed_stmt;
    std::unique_ptr<BaseAST> open_stmt;

    void* toRaw(int n, void* args[]) const override;
};
```

在`if...else...`语句方面，由于涉及到二义性问题，对语法做如下改动：

```ebnf
Stmt        ::= OpenStmt | ClosedStmt;
OpenStmt    ::= "if" "(" Exp ")" Stmt 
              | "if" "(" Exp ")" ClosedStmt "else" OpenStmt
              | "while" "(" Exp ")" OpenStmt;
ClosedStmt  ::= SimpleStmt
              | "if" "(" Exp ")" ClosedStmt "else" ClosedStmt
              | "while" "(" Exp ")" ClosedStmt;
SimpleStmt  ::= ...
```

除了单独拿出来的`symtab`以外，在后端有一些针对内存形式`koopa`的优化而写的全局变量。

由于沿用了文档中给出的模板代码，在我的理念中，`toRaw()`函数返回的应该是这一个`ast`结构所对应的`koopa_raw_value`的指针，如果没有就返回`nullptr`（在这之前，我走过弯路，`toRaw()`的返回值十分不统一，甚至为了不用全局变量，传过`std::vector<koopa_raw_basic_block_data_t*>*`，代码一度十分混乱）。

后来我和解了，单纯追求所谓的模块化是没有意义的，代码应该具有实用性。因此，我们有一个`current_bbs`的全局变量，这是当前函数所构建出来的所有`basic_block`的`vector`，当我们加一条指令的时候，放在缓存中；当加一个`basic_block`或者函数结束时，将缓存中的`raw_value`生成一个`basic_block`加入`current_bbs`

前端也有一些全局变量，不过没什么太多的说法，无非是用来定位指令在栈上位置的`loc_map`，标识当前指令位置的`current_loc`等等，还有用于实现寄存器分配的`reg_info`和`reg_map`

### 2.3 主要设计考虑及算法选择

#### 2.3.1 符号表的设计考虑
`symtab.cpp`中，有用于标识当前作用域层数的`scope_level`，而为了实现多层变量作用域，我们以`std::map`作为每层作用域，然后用`std::stack`作为容器盛装它。每次查找，都在当前层寻找，若找不到就向上找直到最上一层。主要有`query`和`insert`两种方法，一个用来查询，一个用来插入。变量的种类有`CONST`, `VAR`, `FUNCTION`, `ARRAY`, `POINTER`五种。`symbol_val`结构也用了类似`tagged union`的写法

另外，我们在其中也做了对循环的特殊处理，将每个循环的头尾作为一个`std::pair`插入到循环栈中。当进入循环时`push`一个头尾；退出循环则`pop`一次。这样，每次遇到`break`或者`continue`语句，我们在栈顶就可以找到要跳转到的`basic_block`。如果这时栈为空，显然这是非法的语句。

#### 2.3.2 寄存器分配策略
一开始我的策略是，对于有值的指令，通通放在栈上。所有的指令，先把操作数从内存加载到固定的寄存器如`t0`,`t1`上，再做运算，将结果从寄存器再写回内存（栈）中

后来我实现了寄存器分配，但实际上也就是把寄存器作为内存的缓存，最终所有产生值的指令还是要把值写在栈上。这里由于`koopa`是单赋值(SSA)的，所以我们不需要在任何时候把寄存器写回内存。只要我们在产生这条指令的时候把它写到栈上就可以了。

但这样不难发现，它只能解决块内的问题，能提升的效率有限（毕竟每条指令还是要写进内存，这就是一次内存操作了，而我们每条指令的平均内存操作次数一般也不会超过2）

但是我又不愿做$CFG$，只想在块内摆烂分析分析活跃变量，有没有什么比较好的方法呢？我想到了`koopa_raw_value`提供的一个属性：`used_by`。如果一条指令的`used_by.len = 0`，那意味着这条指令不会被任何人使用，那么我们就根本没必要把它放进内存。然后发现并没有什么提升，因为`used_by.len = 0`的无主变量太少了

那么，我们如果一开始不用内存，在指令被驱逐时（不管是因为满了重新分配还是因为失效），再把它写进内存呢？这其实跟前边一开始就把所有指令都写进没差。但我们为了偷懒的创造力是无限的：经过仔细观察，`koopa`不仅有单赋值的特征，对于很多指令，它同时也是“单使用”的。`alloc`本身没有值，不需要放在内存上；而`get_elem_ptr`指令在多维数组的初始化时确实会被使用多次。除此之外我们会发现，剩下的指令，最多被使用一次。

这就为我们的优化提供了依据：我们用上述的思路，但对于这些单使用的指令，只要使用后就将它驱逐出去，而且并不把它存回内存。这次我们的效率有了极大的提高，但中间产生了一个bug，由于我使用了块参数，而由于嵌套，块参数可能直到块结束都没用到，而是在后边的块再用到。鉴于此，我在块开头就把块参数放在了寄存器中。当然了这样还是会有问题，因为如果它被驱逐，就又出问题了。最一劳永逸的办法是为它们分配变量，不过这和不用块参数就没有区别了。

#### 2.3.3 采用的优化策略
优化策略包括上述的寄存器分配优化，以及以下策略：

- 在`koopa`的生成过程中，对块结构做了优化，合并无名块，尽量减少基本块数目。
- 在生成短路部分的代码时，通过对`lhs`能否判断值(`value->kind.tag == KOOPA_RVT_INTEGER`)来减少冗余代码的生成。
- 本来打算直接从`ConstInitVal/InitVal`生成`raw_aggregate`，后来妥协之后改为：如果一个`global var`没有初始值，就把它的初始值设为`zero_init`（实际上，我们可以在很多地方用到`zero_init`，如空的大括号就可以生成对应类型的`zero_init`，但由于我没能成功做到前述，所以这里作罢）。
- 寄存器分配，采用`LRU`策略替换寄存器。
- `get_elem_ptr`，如果`multipler`是2的整数次幂，改用`slli`对`index`操作。

#### 2.3.4 其它补充设计考虑
- 由于各种`koopa_raw_value`需要填充的字段较多，写了一些辅助函数用于构造特定的值。

- 我后来才知道，一个`koopa_raw_value`的`used_by`字段不用自己构建，而提供的库函数在构建`koopa_raw_value`时，会填充这个字段供我们使用。但需要提前写好它的`.kind = KOOPA_RSIK_VALUE`

- 内存形式的一个好处是不用担心重名问题，但它不会在输出中修改函数间基本块的重名基本块名称，这导致我们需要在基本块名称前添加函数名称以区分。不过对于重名的变量，我们的做法是重新构建一遍程序，如下：

在`main.cpp`本来中有一段代码如下，这个`length`的不足也成为了许多bug的来源

```cpp
size_t length = 5000000;
char buffer[length];
koopa_dump_to_string(program, buffer, &length);
koopa_parse_from_string(buffer, &program);
```

实际上，根据`koopa_dump_to_string`的描述，我们可以把代码写成这样：

```cpp
size_t length;
koopa_dump_to_string(program, nullptr, &length);
length += 1;
char *buffer = new char[length];
koopa_dump_to_string(program, buffer, &length);
buffer[length-1] = '\0';
koopa_parse_from_string(buffer, &program);
```

再把`program`解析为`raw_program`，就可以得到一个（至少做了名字）优化后的`raw_program`用于遍历了

## 三、编译器实现

### 3.1 各阶段编码细节

#### Lv1. main函数和Lv2. 初试目标代码生成
这里的`BlockComment`还是有点难写的
```
\/\*[^*]*\*+([^/*][^*]*\*+)*\/
```
#### Lv3. 表达式
从这里开始，从输出字符串变成了构建`raw_program`，开始的时候不知道各个字段该如何填充，非常痛苦

在写`UnaryOp`的识别时，写出了`[!-=]`导致bug，里面的`-`没有被解析为减号，这个模式匹配的是从字符`!`到字符`=`的所有字符

在写`riscv`的输出时，初次接触了寄存器分配，因为这时还没有内存的概念

所有的exp都可能是一个数字，表达式的化简可以在`parser`中做，也可以在`ast`的`toRaw()`函数中做，我们最终选择在后者，是因为前者的代码识别、补全太差，不愿意在里面做太多工作。

这里没有基本块的概念，很难写短路，所以就只是做了等价转换

#### Lv4. 常量和变量

在这里就已经开始用辅助函数构造`koopa_raw_value`了，这里的`make_zero`是因为在`Unary`表达式翻译为`binary`类型的`value`时，总需要加一个0在左边

这里的内存操作指令要注意：`alloc`申请出来的是指向这个类型的指针，`store.dest`与`load.src`都是指针，而`store.value`与`load`的值的类型是指针解引用后的类型。

这里的隐藏样例`multiple_returns`提示了一个原则：一个基本块能且只能以`ret`,`jmp`,`branch`结尾，且其中只能有一个。

这里我终于发现，`used_by`字段是不需要填充的，所以删去了`set_used_by`函数

`riscv`终于涉及了内存，按照文档来就好，这里抛弃了寄存器分配，以稳妥起见，把所有的指令都放在了栈上；实现了符号表

#### Lv5. 语句块和作用域

这里符号表变成了多层的，使用`vector`把各个`map`组织起来

在`lv5`把对表达式的计算、符号的定义都放在了`ast.cpp`中，而从`parser`中排除了。这里的原因如下：在如果在`parser`中直接解析变量，我们没法知道它在第几层，因为还没进行`Block`的归约。所以我们只能改架构，把这一部分放在`toRaw()`的时候做

从这里开始，函数返回值在歪路上一去不复返了，现在返回`value`的`vector`，之后返回`basic_block`甚至`basic_block`的`vector`，越来越麻烦，就是不肯用全局变量导致的

#### Lv6. if语句

这里我走了许多弯路，我的结论是：如果自己构造数据结构，可以沿用上课讲的方法：放标签，最后再去构造`basic_block`；如果就用原汁原味的`koopa.h`提供的数据结构，我们可以用一个空的但有名字的`block`去代替一个标签，只要最后把这些冗余合并起来就好了。

对于短路，这里贴一个我写的解决方案：

```
 lhs && rhs
                         false -->  0
                        /
                    能                           
                /       \                           /  能   --> lhs && rhs
lhs能否判断值？           true  -->  rhs能否判断值？ 
                \                                   \  不能 --> ne rhs, 0
                    不能:
                            ...(lhs)
                            br lhs %rhs_entry %end(0)
                        %rhs_entry:
                            ...(rhs)
                            %0 = ne rhs 0
                            jmp %land_end(%0)
                        %land_end(%value: i32):
                            %value 即为表达式的值
```

但实际上，只写最后这段代码，也是没有任何问题的

这里的`riscv`生成涉及到了`imm12`的问题，需要注意

#### Lv7. while语句

如何解决`break`和`continue`寻找跳出点的问题在前述已经讲过，但刚开始写的时候，这里是把两个`basic_block`用参数向下传递的，比较dirty

这也开创了给`toRaw()`函数添加怪参数的先河，并荼毒至今，实际上，一些合理的全局变量才是更好的解决方案

#### Lv8. 函数和全局变量

符号表中新增了函数类型

对于语法中的`{}`（重复0次或若干次），我们在写`parser`的语法规则时并不能这么写。可以定义一种新的`ast`类，但我在此是定义了一种新的`token`类型，可以说是`vector of ast`。这样，我们不用在`ast.h`中再新加一种`ast`，还要定义其属性、方法，而可以很方便地直接把这个列表解析出来

由于`koopa_raw_slice`中的`buffer`是一个`const`指针，所以想对其`realloc`的想法是无法成功的

从这里开始，在语法中引入了`C99`的指定初始化器，用来方便、直观地填充各种属性，同时终于对项目的`toRaw()`函数进行了重构，现在它不会再在函数间传各种怪东西了

另外现在的项目设计有一个核心理念：一个`ast`结构不需要对自己的孩子负责，它只需要调用获得需要的值即可，存在控制链，但尽量不出现访问链

#### Lv9. 数组

在通过不完整的数组构造`aggregate`的过程中，我写出了一个很严重的bug，但它居然能通过$31/34$的样例。在数组的初始值中，只要有一层括号的嵌套，它的维数信息就应该少一个，但我没去对这个`vector`做`pop_back`。这样会导致以下的数组解析错误：

```c
int arr[2][3] = {{}, 1, 2, 3}; // 应解析为{0, 0, 0, 1, 2, 3}
```

但我这里解析到里层的大括号时，其最长维度是$6$，又和$0$对齐，所以就用6个0把数组填满了。或者说，它认为这是一行错误的语句。

我是在树洞上搜索，发现有人提醒了这个问题，回来用这个样例测试才发现问题的

在`riscv`这里，关于`alloc`有一些想法：对于有值的指令，它们的值存在栈上，这没问题。但到`alloc`这里出问题了。在以前，一个`alloc`申请的空间是$4$字节，这和一条指令所占的空间是一样的，也就导致了一种误解，`alloc`的值就是这里写的这个变量的值。但实际上`alloc`的值应该是指向这片空间的指针。然而因为我们的数组就在栈上，它的位置可以随取随算，我们确实也没有为它的值单开一块空间的必要。不过这样就会导致代码比较凌乱

我们当然可以把`alloc`当成一条一般的具值指令，为它分配栈空间，并在这里手动写上它申请空间的起始地址，不过这样的话，我们每次使用这个地址就都要从内存中取了，这显然会降低效率

### 3.2 工具软件介绍
1. `flex/bison`：使用库进行语法分析，生成`ast`树
2. `libkoopa`：用来构建`koopa_program`，并生成中间代码，同时也可以用它读取一份`koopa`中间代码，用`gdb`查看其内存形式用于指导`raw_program`的构建
3. `kira-rs`：作为生成`koopaIR`的参考

### 3.3 测试情况说明

由于有些时候样例过于复杂，很难debug，我会想办法做减法，定位到出问题的某一句，再去仔细思考为什么出错

课件中提到，$\mathrm{SB}$型指令的最大跳转范围是$(-4\mathrm{KB}, +4\mathrm{KB})$，而$\mathrm{UJ}$型指令则是$(-1\mathrm{MB}, +1\mathrm{MB})$，这就引入了一个问题：在某些变态长的代码中，`bnez`指令的步子迈得太大了。我的解决方案是再加一级跳板，用`j`来完成

## 四、实习总结

### 4.1 收获和体会

对我来说，最大的收获就是这个`mini compiler`，它虽然是个玩具编译器，但五脏俱全，让我理解了代码到中间代码再到机器代码的过程以及其中的具体细节

虽然选择用`C`构建内存形式的`KoopaIR`在一开始遇到了不计其数的困难，但当我完成这个编译器时，我对`KoopaIR`，以及其中值的字段有了更深刻的理解，也逐渐理解了这一套设计背后的理念

### 4.2 对实习过程和内容的建议

希望文档能不断更新，并且希望文档能对内存形式的`KoopaIR`做一些说明。即使在生成`koopa`的过程中不使用内存形式，在构建`riscv`文件时也需要理解它们的属性，这一点我认为还是很重要的

写内存形式的`KoopaIR`，负反馈太严重了。字符串的话，只要没有跑飞/遇到`assert`，总是有输出的，但内存形式的`KoopaIR`只要构建过程中有一点不合规范，那就是零输出。debug非常痛苦，需要`gdb`一点一点看变量，尤其这其中还有很多`void*`指针，查看值往往需要手动强制转换，非常麻烦

希望文档不要惜墨如金，至少把涉及的内容尽量每个都举个例子，全靠猜是写不出正确的程序的，这一点在`lv9`中尤甚。

另外，个人认为隐藏样例不是什么好的设计。这种项目设计和算法题还是有很大的差距的，没必要拿一些大家想一辈子也想不出来的东西来为难人(e.g. 为函数补充`return`语句，大家没准以为这是$\mathrm{UB}$呢)