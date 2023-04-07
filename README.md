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
  
  
## Installation.
1. Download the release Zip from here.
2. Decompress the folder.
3. Add it to your RenderStream Projects folder.
4. Add a new workload in disguise.
5. Select SpoutRenderstream.
