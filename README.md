# Shader Compiler

## Supported Matrix
|Vendor  |Driver                |Version    |Machine                      |Note                 |
|--------|----------------------|-----------|-----------------------------|---------------------|
|AMD     |GPU ShaderAnalyzer    |11.7 ~ 12.4|R600 ~ RV970                 |Disassembly only     |
|AMD     |Catalyst              |16.3 ~     |GCN ~ GCN4                   |                     |
|ATI     |Radeon                |133.1      |R200                         |From WDK 6001        |
|ARM     |Mali Offline Compiler |6.4.0      |Utgard / Midgard / Bifrost   |Disassembly from Mesa|
|NVIDIA  |KelvinR7              |R7         |NV20                         |From Xbox            |
|NVIDIA  |NVShaderPerf          |101.31     |NV30 ~ G70                   |                     |
|NVIDIA  |NVShaderPerf          |174.74     |NV40 ~ G86                   |                     |
|Qualcomm|Adreno Shader Compiler|DX09.02.02 |Oxili / A4x / A5x / A6x / A7x|Disassembly from Mesa|
