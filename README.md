# GuanDan
##### 本项目由Si-xiyu完成，系华南理工大学24级C++实训作业，仅供学习交流，请勿用于学术造假或商业用途。
##### This project was completed by Si-xiyu as a C++ training assignment for the Class of 2024 at South China University of Technology(SCUT). It is for academic exchange only and should not be used for academic fraud or commercial purposes. 
欢迎报考华南理工大学OwO（）
# 功能介绍
本项目实现掼蛋游戏的逻辑，并提供GUI界面和人机对战的逻辑，以及数据库和局域网联机的接口。
此模式需要两副牌（每一副牌包含2到A各4张，包括红桃、黑桃、红方、梅花四个花色，大小王各1张，共54张），**初始级牌为2**，具有下列牌型：

1.  单牌：牌的大小顺序为：大王、小王、级牌、A、K、…、2.

2.  对子：大小顺序与单牌相同，最大的对子为对大王。

3.  三张牌可以是三不带，也可以是三带一对，但不可以三带一。

4.  四张及以上是炸弹，张数多则炸弹大。

5.  钢板：连续的两组三张牌。例如555666.与斗地主不同，2 可以出现在钢板中，如AAA222和222333.最大的钢板为KKKAAA.

6.  顺子：连续的**五张**单牌。与斗地主不同，2可以出现在顺子中，如A2345和23456.最大顺子为10JQKA.花色相同的顺子为**同花顺**，5炸<同花顺<6炸.

7.  连对：连续的**三个**对子。与斗地主不同，2可以出现在连对中，如AA2233和223344.最大连对为QQKKAA.

8.  癞子规则：**红桃**级牌为癞子，可以替代除大小王以外的任何牌。

9.  升级规则：一二位升三级，一三位升两级，一四位升一级。最后一个出完牌的玩家向第一个出完牌的玩家进贡；进贡的牌为自己手上最大的牌，吃贡的玩家还一张10以下的牌给进贡者。若两张大王同在输家手中，或输家手中没有大过A的牌则不用进贡。

# 软件架构
本项目采用MVC架构，通过GD_Controller类控制游戏主流程。  
**Module**包括Card，Team，Player等用于数据类，以及CardCombo等方法类。
**View**包括CardWidget，PlayerWidget，GuanDan等类，用于负责视图逻辑。

# 开发环境
本项目基于VS2022 + Qt Tool插件开发，基于Qt Creator开发的项目https://gitee.com/Si-xiyu/qt_-guan-dan
