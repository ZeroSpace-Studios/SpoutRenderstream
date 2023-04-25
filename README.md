![logo-narrow-dark](https://user-images.githubusercontent.com/32549017/230673150-8743de1a-b76c-47eb-b707-3ac023ef5169.png)

![image](https://user-images.githubusercontent.com/32549017/230662065-5dfc1772-e491-489e-a401-358b7885f2cc.png)

# SpoutRenderstream
Allow running any spout compatible application on a Disguise RX at full 12bit RGBA.

Useful for
1. Large LED or projection installations where realtime projects need to run across large canvases.
2. Receiving uncomrpessed textures from disguise.
3. Color grading and previs.


## Features:
-  High Performance
  - Easily runs 6k at 60ps over 25gb interconnect.
- Supports any arbitrary canvas size.
  - Automatically scales content to renderstream requested size.
  - Handles color format conversion on-the-fly.
- Allows for texture streaming out of disguise.
  - Exposes any texture parameters as spout inputs on RX.
  
  
## Installation:
1. Download the release Zip from [here](https://github.com/ZeroSpace-Studios/SpoutRenderstream/releases/download/v1.7/ZeroSpaceSpoutBridge.zip).
2. Decompress the folder.
3. Add it to your RenderStream Projects folder.
4. Add a new workload in disguise.
5. Select SpoutRenderstream.

## Tutorial
https://youtu.be/hQgp9176t4M


## Notes
To expose texture outputs from disguise you need to add a custom argument in disguise like you would for unreal and set it to `--inputs` and that will enable the feature. This is disabled by default to maiximize performance.
