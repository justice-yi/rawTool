# ImageTool-Qt — Raw 图查看工具（逆向复刻版）

对 `ImageTool0_360R.exe`（三星 System LSI ImageTool v0.360，2004-2006）的
行为级复刻：**C 语言核心 + Qt Widgets 界面壳**，无死代码、无死参数、全模块化。

## 功能（对应原程序）

| 功能 | 原程序出处 | 复刻实现 |
|---|---|---|
| 打开 RAW（.raw/.byr/.dat/.sraw） | Open(raw) 菜单 + Raw Format 对话框 | `core/raw_loader.c` + `gui/RawFormatDialog` |
| 位深 8/10/16、HL/LH 字节序、已知分辨率推断 | DIALOG 182 / .data 内置表 | `raw_loader.c` |
| Bayer 排列 RGGB/GRBG/GBRG/BGGR | PAT_SEL 参数 | `core/bayer.c` |
| ISP 流水线（钳位→白平衡→Shading→BPR→GRGB→二级增益→矩阵→Gamma→Y） | 0x404d20 驱动 + ~200 个 ISP_P1 参数 | `core/isp_pipeline.c` + `gui/IspPanelDialog` |
| 阶段调试转储（00_d_clamp_out…12_shade_y） | 0x40d110（P2 ASCII PPM / raw） | `isp_pipeline.c` 内置 dump |
| 直方图 / Mean / Std / SNR / 摘要导出 | 0x4338b0 hist_summary.txt | `core/analysis.c` + `gui/AnalysisDialog` |
| CSV 统计导出（替代 Excel OLE） | 0x44a220 Excel 自动化 | `core/analysis.c` `it_stat_write_csv` |
| 缩放 / 1:1 / 像素取色（Bayer_%s of %s / RGBA） | 0x421850 | `gui/ImageView` |
| 拖拽打开文件（按扩展名分流 RAW/图像） | — | `gui/MainWindow` drag&drop |
| known size 下拉（内置传感器尺寸表） | .data 内置分辨率表 | `gui/RawFormatDialog` |
| Bypass ISP 原始预览（仅 demosaic 对比） | 原预览路径 | `gui/MainWindow` + `core/isp_pipeline.h` `it_isp_preview` |
| 悬停放大镜（15×15 最近邻 ×10，可开关） | — | `gui/ImageView` |
| 仿真：丢首行 N 行重解析 | sensor 断流调试场景 | `core/itypes.h` `it_image_crop` + `gui/MainWindow` 工具栏 |
| 仿真：按任意 pattern 重解析 | PAT_SEL 错配调试 | `gui/MainWindow` 工具栏 |
| 框选区域 SNR 掩码 | DIALOG 178 区域选择 | `gui/ImageView` + `core/analysis.c` |
| RGB 前置滤波（绿中值 + Sigma 边缘保持） | rgb_prefilter 阶段 | `core/isp_pipeline.c` |
| BPR 半径级联确认（3×3→15×15） | BPR_TH1/3/5/7 | `core/isp_pipeline.c` |
| RAW dummy 行/列中心裁剪 | 1288×1032 等带 dummy 输出 | `core/raw_loader.c` |
| 记忆上次目录与 RAW 格式（QSettings） | 注册表 OPEN_DIR 持久化思路 | `gui/MainWindow` |
| INI 参数保存/加载（原 ISP_P1 节参数名） | 0x40f2d0 参数注册表 | `core/it_persist.c` |
| BMP/PPM/RAW16 保存 | Supported Files 过滤器 | `core/imgio.c` |

模块说明与逆向证据详见 **[docs/模块流程文档.md](docs/模块流程文档.md)**。

## 构建

依赖：CMake ≥ 3.16、Qt 5.15+/6.x（Widgets）、C11/C++17 编译器。

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build        # 核心库端到端冒烟测试
./build/ImageToolQt           # 运行
```

### 交叉编译 Windows 单文件 exe（WSL Ubuntu，静态 Qt）

```bash
# 依赖：apt install mingw-w64 cmake；Qt 静态交叉构建于 /opt/qt6-mingw
#（host moc 在 /opt/qt6-host/libexec）
cmake -B build-win   -DCMAKE_TOOLCHAIN_FILE=toolchains/mingw64-cross.cmake   -DQt6_DIR=/opt/qt6-mingw/lib/cmake/Qt6   -DCMAKE_PROGRAM_PATH=/opt/qt6-host/libexec   -DCMAKE_BUILD_TYPE=Release
cmake --build build-win --target ImageToolQt -j
x86_64-w64-mingw32-strip build-win/ImageToolQt.exe
```

产物 `build-win/ImageToolQt.exe`：**单文件 ~20MB**，静态链接 Qt 与 MinGW
运行库（`-static` + GUI 子系统），仅依赖 Windows 系统 DLL，无需任何部署。

## 目录结构

```
ImageTool-Qt/
├── CMakeLists.txt
├── docs/模块流程文档.md        # 逆向结论 + 模块流程 + 地址对照表
├── src/
│   ├── core/                  # 纯 C 核心（不依赖 Qt）
│   │   ├── itypes.[ch]        # 类型 / 配置 / 默认值
│   │   ├── raw_loader.[ch]    # RAW/Bayer 文件加载
│   │   ├── bayer.[ch]         # Bayer → RGB 解马赛克
│   │   ├── isp_pipeline.[ch]  # ISP 流水线（含阶段转储）
│   │   ├── analysis.[ch]      # 统计 / SNR
│   │   ├── it_persist.[ch]    # ISP 参数 INI 保存/加载（原 ISP_P1）
│   │   └── imgio.[ch]         # BMP / PPM / RAW16 写出
│   └── gui/                   # Qt 壳（仅事件与展示）
│       ├── main.cpp
│       ├── MainWindow.[ch]    # 菜单 / 状态栏 / 流程编排
│       ├── ImageView.[ch]     # 缩放视图 + 取色
│       ├── RawFormatDialog.[ch]
│       ├── IspPanelDialog.[ch]
│       └── AnalysisDialog.[ch]
└── tools/smoke_test.c         # 核心库端到端测试
```

## 设计原则

- **核心/GUI 分离**：全部算法在纯 C 的 `itcore` 静态库里，可单独测试
  （`it_smoke_test`）；Qt 层不参与像素运算。
- **无死代码**：每个函数、每个参数都有实际消费方；测试中的调用一律用
  `expect()/require()` 显式检查（避免 assert 在 Release 下被编译掉）。
- **逆向保真**：阶段顺序、定点语义（CISGAIN Q5、增益 Q8）、转储文件名、
  统计摘要格式、取色文本均与原程序一致；有意差异（MDI→单窗口、
  Excel OLE→CSV 等）在文档 §3.4 中列明。
