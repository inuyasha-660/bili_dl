# BILI_DL
基于配置的 Cli BiliBili 视频下载工具

## 功能
- [x] 视频/音频下载(Bvid)
- [x] 收藏夹
- [ ] 系列
- [ ] 番剧
- [ ] 合集

## 用法
``bili_dl <CONFIG>``

### 配置编写
使用 **json** 作为配置文件

#### 公共参数
- ``SESSDATA``: 储存于Cookie中，用于请求鉴权
- ``MaxThread``: 可使用的最大线程数
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

各参数值的意义可以参考 [视频流URL(BAC Document)](https://socialsisteryi.github.io/bilibili-API-collect/docs/video/videostream_url.html)

``````json
"Require": [
    {
            "Bvid": "BV1794y1t7nw",
            "part": [
                0
            ],
            "mode": 2,
            "qn": "80",
            "audio": "30280",
            "coding": "12"
        },
        {
            "Bvid": "BV1kt411z7ND",
            "part": [
                1,
                2
            ],
            "mode": 1,
            "qn": "*",
            "audio": "*",
            "coding": "*s"
        }
  ]
``````
当仅下载音频时(mode = 2)， ``coding``对象将不会被匹配，但需要保留

查看示例： [examples/video1.json](/examples/video.json)

##### 收藏夹 (Type = 2)
- ``fid``: 目标收藏夹 fid，请确保有权限访问
- ``mode``/``qn``/``audio``/``coding`` 含义同上
- ``except``: 此对象中的视频配置将覆盖全局配置

``````json
  "Require": {
    "fid": "3566286331",
    "mode": 0,
    "qn": "*",
    "audio": "*",
    "coding": "*",
    "except": [
      {
        "Bvid": "BV1rW411S7N9",
        "part": [1],
        "mode": 2,
        "qn": "*",
        "audio": "*",
        "coding": "*"
      }
    ]
  }
``````

## 从源码构建
### 依赖
- [libcurl](https://curl.se/libcurl/) ([Curl LICENSE](https://curl.se/docs/copyright.html))
- [cJSON](https://github.com/DaveGamble/cJSON) ([MIT license](https://github.com/DaveGamble/cJSON/blob/master/LICENSE))
- [C-Thread-Pool](https://github.com/Pithikos/C-Thread-Pool) ([MIT license](https://github.com/Pithikos/C-Thread-Pool/blob/master/LICENSE))
- [ffmpeg](https://www.ffmpeg.org)(libavcodec, libavformat) ([LICENSE](https://www.ffmpeg.org/legal.html))
- [md5-c](https://github.com/Zunawe/md5-c) ([Unlicense license)](https://github.com/Zunawe/md5-c/blob/main/UNLICENSE]))

### Meson
``````sh
meson setup build
cd build
meson compile
``````
生成可执行文件 ``bili_dl``

所用API来自 [bilibili-API-collect](https://github.com/SocialSisterYi/bilibili-API-collect)

## 许可证
GPL-3.0 License， 详细信息 [LICENSE](./LICENSE)
