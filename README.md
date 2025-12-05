# BILI_DL
基于配置的轻量化 BiliBili 视频下载器

## 用法
``bili_dl <CONFIG>``

### 配置编写
使用 **json** 作为配置文件

#### 公共参数
- ``SESSDATA``: 储存于Cookie中，用于请求鉴权
- ``MaxThread``: 可使用的最大线程数字
- ``Type``: 目标类型(1: 视频)
- ``Output``: 输出目录

``````json
{
  "SESSDATA": "<string>",
  "MaxThread": <int>,
  "Type": <int>,
  "Output": "<string>",
}
``````

#### 请求数据
所有目标请求数据存放于 ``Require`` 数组下

##### 视频下载 (Type = 1)
- ``Bvid``: 目标视频 Bvid
- ``part``: 目标分P，从**1**开始(0: 全选)
- ``mode``: 目标类型(0: 视频+音频 1: 视频 2: 音频)
- ``qn``: 目标视频质量(**"*"**: 使用默认值)
- ``audio``: 目标音频质量(**"*"**: 使用默认值)
- ``coding``: 目标编码((**"*"**: 使用默认值))

``````json
"Require": [
    {"Bvid": "BV14EUHB8EXr", "part": [0], "mode": 0, "qn": "80", "audio": "30280", "coding": "12"},
    {"Bvid": "BV1kt411z7ND", "part": [1, 2], "mode": 1, "qn": "74", "audio": "30280", "coding": "7"}
  ]
``````
当仅下载音频时(mode = 2)， ``coding``对象将不会被匹配，但需要保留

查看示例： [examples/video1.json](/examples/video1.json)

## 从源码构建
### 依赖
- [libcurl](https://curl.se/libcurl/)
- [cJSON](https://github.com/DaveGamble/cJSON)
- [C-Thread-Pool](https://github.com/Pithikos/C-Thread-Pool)

``thpool.c``,``thpool.h``来自 [C-Thread-Pool](https://github.com/Pithikos/C-Thread-Pool)  **MIT license**

### Meson
``````sh
meson setup build
cd build
meson compile
``````
生成可执行文件 ``bili_dl``