# UE_COURSE_OF_TECENT
## MultiPlayerFPSDemo
腾讯游戏客户端公开课demo，本课程demo仍处于施工中，目前为第六次作业提交时的版本。
### 第四节课作业演示视频链接：   
链接：https://pan.baidu.com/s/1EscDx5jBZcrcr93gP-6BEw?pwd=lfov 
提取码：lfov 
### 第五节课作业演示视频链接：   
链接：https://pan.baidu.com/s/1M_km5sxvpDIVSCrVdEUH1Q?pwd=nbid 
提取码：nbid 
### 第六节课（基本物理）作业演示视频链接：   
链接：https://pan.baidu.com/s/1Cc3tMbIGp0vIZtlnaLqa6A?pwd=i883 
提取码：i883 
### 第七节课（渲染基础）作业演示视频链接：   
链接：https://pan.baidu.com/s/1DUdK3dE-3rQxeyhig3Jekw?pwd=42kx 
提取码：42kx 
### 当前实现的功能
1. UI开发
    * 玩家跳跃和开火的按钮UI。
    * 武器十字线准星的UI，开火时准星的扩散动画。
    * 玩家的血条UI，玩家受击时显示掉血的UI，玩家血量低于20%时血条和数字颜色的变化。
    * 玩家当前武器的弹夹子弹数目和总子弹数目的UI。
    * 移动端左摇杆移动和右摇杆转动视角UI的绑定。
2. 玩家开发
    * 玩家类的创建，包括自定义组件结构，碰撞设置，输入绑定（移动旋转），静步（Shift），跳跃(Space)，开火(鼠标左键）。
    * 玩家的生命值和受击逻辑。
    * 玩家不同物理部位应用不同的受击伤害。
    * 按TAB键切换第一人称和第三人称视角。
    * 按V键进行近战攻击
    * 按G键进行投掷爆炸物发射（仅装备有武器时可发射，目前暂时从枪口发射）
    * 玩家移动时发出脚步声并在地面上留下脚印
3. 武器开发
    * 第一人称客户端武器类的编写。
    * 第三人称服务器端武器类的编写。
    * 服务端武器的拾取以及拾取后的第一人称和第三人称的显示。
    * 玩家初始自带武器。
    * 玩家初始不自带武器时拾取武器。
    * 武器开火。
    * 武器的弹药逻辑和同步。
    * 子弹的射线检测
    * 狙击枪的开关镜
    * 不同武器的不同后坐力
4. 动画的开发
    * 玩家的第三人称移动动画（供其他玩家以及三人称视角下的自己观看），玩家第一人称的手臂动画（供玩家自己观看）。
    * 玩家瞄准时的俯仰动画（目前还未实现同步，只在自己客户端可见）
    * 武器开火的手臂抖动（第一人称）以及身体抖动（第三人称）。
    * 武器开火时的枪体动画（第一人称客户端）以及枪体粒子效果(第一人称客户端和第三人称服务端可见）。
    * 近战攻击时的动画（第三人称多端可见）
5. 其他效果的开发
    * 开火时的镜头抖动。
    * 射击声音。（自己听到的2D声音和其他人听到的3D声音）
    * 弹孔生成。
    * 投掷物的效果（命中玩家直接爆炸，否则会经过一段固定时间再爆炸，对范围内所有玩家造成高额伤害，投掷物碰到不同的物理材质会有不同的表现，如碰到地面速度会受到明显的阻尼，碰到墙壁则会弹的更远）




