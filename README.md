# BILI_DL
``bili_dl <CONFIG>``

基于配置的轻量化 BiliBili 视频下载器

## 目标配置
使用 **json** 作为配置文件

### 公共参数
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

### 请求数据
所有目标请求数据存放于 ``Require`` 数组下

#### 视频下载 (Type = 1)
- ``Bvid``: 目标视频 Bvid
- ``part``: 目标分P，从**1**开始
- ``mode``: 目标类型(0: 视频+音频 1: 视频 2: 音频)
- ``qn``: 目标视频质量
- ``audio``: 目标音频质量
- ``coding``: 目标编码

``````json
"Require": [
    {"Bvid": "BV14EUHB8EXr", "part": [0], "mode": 0, "qn": "80", "audio": "30280", "coding": "12"},
    {"Bvid": "BV1kt411z7ND", "part": [1, 2], "mode": 1, "qn": "74", "audio": "30280", "coding": "7"}
  ]
``````
当仅下载音频时(mode = 2)， ``coding``对象将不会被匹配，但需要保留

## 从源码构建
### 依赖
- [libcurl](https://curl.se/libcurl/)
- [cJSON](https://github.com/DaveGamble/cJSON)
- [C-Thread-Pool](https://github.com/Pithikos/C-Thread-Pool)

``thpool.c``,``thpool.h``下载自``https://github.com/Pithikos/C-Thread-Pool``

### Meson
``````sh
meson setup build
cd build
meson compile
``````
生成可执行文件 ``bili_dl``