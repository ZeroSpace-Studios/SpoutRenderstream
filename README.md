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

### Licenses

#### Spout
BSD 2-Clause License

Copyright (c) 2020, Lynn Jarvis
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

