# 附录：ISP_P1 INI 全参数键表

`ImageTool-Qt` 复刻版 INI（节 `[ISP_P1]`）的全部键，与 `ItConfig` 字段、
所属流水线级、默认值的完整对照。键名 = 原程序 .data 节的参数符号名
（Ghidra 反编译 0x40f2d0 注册函数）。

- 增益定点语义：CISGAIN 系 Q5（32=×1.0），WB/二级增益系 Q8（256=×1.0）
- 阈值按 10bit 语义定义；数据量程 full_scale≠1023 时运行期按
  `th × full_scale / 1023` 等比换算（见模块文档 §3.2）
- `it_config_load_ini` 未出现的键保持现值；无法识别的键计入 unknown 计数

| INI 键 | ItConfig 字段 | 所属级 | 默认 | 说明 |
|---|---|---|---|---|
| PreprocessTh | preprocess_th | 钳位 | 2 | 黑电平以下切除（10bit 语义） |
| PAT_SEL | pat_sel | 全局 | 0 | 0=RGGB 1=GRBG 2=GBRG 3=BGGR |
| DCLP_ON | dclp_on | 钳位 | 0 | 数字钳位 + CIS 增益使能 |
| DCLP_STRT_H | dclp_strt_h | 钳位 | 0 | 有效区起始列（之前为 dummy 区） |
| DCLP_STRT_V | dclp_strt_v | 钳位 | 1 | 有效区起始行 |
| POFFSET | poffset | 钳位 | 0 | dummy 区像素偏移 |
| CISGAIN_GR | cisgain_gr | 钳位 | 32 | Gr 通道增益（Q5） |
| CISGAIN_GB | cisgain_gb | 钳位 | 32 | Gb 通道增益（Q5） |
| CISGAIN_R | cisgain_r | 钳位 | 32 | R 通道增益（Q5） |
| CISGAIN_B | cisgain_b | 钳位 | 32 | B 通道增益（Q5） |
| POFFSET_GR | poffset_gr | 钳位 | 0 | Gr 通道偏移 |
| POFFSET_GB | poffset_gb | 钳位 | 0 | Gb 通道偏移 |
| POFFSET_R | poffset_r | 钳位 | 0 | R 通道偏移 |
| POFFSET_B | poffset_b | 钳位 | 0 | B 通道偏移 |
| BPR_ON | bpr_on | BPR | 0 | 坏点校正使能 |
| BPR_TH1 | bpr_th1 | BPR | 0 | 3×3 窗检出阈值 |
| BPR_TH3 | bpr_th3 | BPR | 0 | 7×7 窗确认阈值 |
| BPR_TH5 | bpr_th5 | BPR | 0 | 11×11 窗确认阈值 |
| BPR_TH7 | bpr_th7 | BPR | 0 | 15×15 窗确认阈值 |
| GRGB_ON | grgb_on | GRGB | 0 | 绿不平衡校正使能 |
| GRGB_GRB | grgb_grb | GRGB | 0 | 校正方向 |
| GRGB_AVG | grgb_avg | GRGB | 0 | 1=整行 Gb 校正；0=局部准则 |
| GTHR01 | gthr01 | GRGB | 0 | 行差门槛 / 局部偏差门槛 |
| RGB_FILTER_ON | rgb_filter_on | 前置滤波 | 0 | RGB 边缘保持平滑使能 |
| RGB_GMEDIAN_ON | rgb_gmedian_on | 前置滤波 | 0 | G 通道 3×3 中值使能 |
| SIG01_CONST | sig01_const | 前置滤波 | 0 | σ 常数项 |
| SIG01_MULT | sig01_mult | 前置滤波 | 0 | σ 亮度项（Q8） |
| SIG02_MULT | sig02_mult | 前置滤波 | 0 | 空间衰减（Q8） |
| SHADE_ON | shade_on | Shading | 0 | Shading 校正使能 |
| SHADE_RADIAL | shade_radial | Shading | 0 | 1=径向；0=3×3 分窗 |
| SHADE_XC | shade_xc | Shading | 0 | 径向中心 x（0=自动中心） |
| SHADE_YC | shade_yc | Shading | 0 | 径向中心 y（0=自动中心） |
| GAIN_H1..H9 | gain_h[9] | Shading 分窗 | 0 | 3×3 分窗各列增益（GUI 统一填值） |
| GAIN_V1..V9 | gain_v[9] | Shading 分窗 | 0 | 3×3 分窗各行增益（GUI 统一填值） |
| CRWB | wb_gain_r | 白平衡 | 256 | R 增益（Q8） |
| CGWB | wb_gain_g | 白平衡 | 256 | G 增益（Q8） |
| CBWB | wb_gain_b | 白平衡 | 256 | B 增益（Q8） |
| CRDS | wb_ds_r | 白平衡 | 0 | R 暗电平（先扣后增益） |
| CGDS | wb_ds_g | 白平衡 | 0 | G 暗电平 |
| CBDS | wb_ds_b | 白平衡 | 0 | B 暗电平 |
| gain_r_2nd | gain_r_2nd | 二级增益 | 256 | R 增益（Q8） |
| gain_g_2nd | gain_g_2nd | 二级增益 | 256 | G 增益（Q8） |
| gain_b_2nd | gain_b_2nd | 二级增益 | 256 | B 增益（Q8） |
| offset_r_2nd | offset_r_2nd | 二级增益 | 0 | R 偏移 |
| offset_g_2nd | offset_g_2nd | 二级增益 | 0 | G 偏移 |
| offset_b_2nd | offset_b_2nd | 二级增益 | 0 | B 偏移 |
| GAM_SEL | gam_sel | Gamma | 0 | 0=关；1=启用 gamma_value |
| GammaValue | gamma_value | Gamma | 1.0 | 幂指数（γ=2.2 → 值 2.2） |
| RGM10 | rgm[0][0] | 颜色矩阵 | 1 | 矩阵 (0,0)，Q8 |
| RGM11 | rgm[0][1] | 颜色矩阵 | 0 | 矩阵 (0,1) |
| RGM12 | rgm[0][2] | 颜色矩阵 | 0 | 矩阵 (0,2) |
| RGM20 | rgm[1][0] | 颜色矩阵 | 0 | 矩阵 (1,0) |
| RGM21 | rgm[1][1] | 颜色矩阵 | 1 | 矩阵 (1,1) |
| RGM22 | rgm[1][2] | 颜色矩阵 | 0 | 矩阵 (1,2) |
| RGM30 | rgm[2][0] | 颜色矩阵 | 0 | 矩阵 (2,0) |
| RGM31 | rgm[2][1] | 颜色矩阵 | 0 | 矩阵 (2,1) |
| RGM32 | rgm[2][2] | 颜色矩阵 | 1 | 矩阵 (2,2) |
| RY_COEF | ry_coef | Y 生成 | 77 | Y 权重 R（Q8，0.299） |
| GY_COEF | gy_coef | Y 生成 | 150 | Y 权重 G（Q8，0.587） |
| BY_COEF | by_coef | Y 生成 | 29 | Y 权重 B（Q8，0.114） |

**未入 INI 的运行项**（由加载流程决定，不持久化）：调试转储开关
（dump_ppm/dump_dclamp 等，IspPanelDialog 会话内设置）。

**原程序有而复刻版未实现行为对应的键**：SH_VAL_R0..B5、
SHADE_YON、SHADE_MODE、GCOTH_H/V、RBTHR、Thresh_Tim、Unit/Sum/Final00..01、
SIG02 域扩展等（见 rea_work/param_table.txt 原表）——这些属于原 Shading
分窗/统计域的细粒度参数，复刻版以 SHADE_XC/YC/SH_VAL 中心-角点近似
替代，属 §3.4 有意差异。
