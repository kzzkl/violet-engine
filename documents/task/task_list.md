# Task lisk

|任务名|模块|描述|
|---|---|---|
|root|task|根任务，标志开始新的游戏循环|
|window tick|window|只能在主线程执行，调用 window::tick，处理窗口信息|
|game logic start|task|所有游戏逻辑任务都需要依赖这个任务，标志开始处理游戏逻辑|
|game logic end|task|这个任务会依赖所有游戏逻辑任务，标志游戏逻辑处理完毕|
|graphics render|graphics|渲染主摄像机并写入屏幕后缓冲，调用 graphics::render、graphics::present|
|ui tick|ui|触发 UI 控件事件、处理 UI 顶点更新等|
|editor tick|editor|编辑器逻辑，更新编辑器 UI 等|
