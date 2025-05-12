Created by Mark Simpson for Computer Games Technology BSc (Hons) Final Year Project

System Requirements: 
NVIDIA graphics card with Shader Model 3.0.  
Depth stencil render targets are (I believe) an NVIDIA only feature.  I also use the vPos register which is part of Shader Model 3.0.

Run the application by double clicking on the “Win32.exe” file.  Also, if NVPerfHUD is available, the application may be started with the NVPerfHUD driver by dragging the win32.exe icon over the NVPerfHUD icon.  It should be noted that switching renderers results in the deferred renderer’s performance suffering.  This is due to the way in which the resources are created.  If benchmarking is required, it is recommended that the user quit the application and restart it to properly re-initialise the deferred renderer.

Controls (can be accessed using the help-controls menu):

Action						Key / Button
Move camera position				W A S D
‘Run’						Hold left shift while moving camera
Rotate camera					Move mouse & hold mouse 2
Change renderer					R
Toggle directional lights			I
Toggle omni-directional lights			O
Toggle spot lights				P
Toggle bounding sphere visualisation		B
Toggle light hull visualisation			L
Toggle debug shadow map display			Return
Display render targets (Deferred Shading only)	Numpad 0, 1, 2 to display MRTs 0, 1 and 2 respectively.
Display final image (Deferred Shading only)	Numpad 3

The following command line arguments are available, with the options following in brackets:

Set the default renderer:
-renderer [deferred | forward | dummy]

e.g. To load the Deferred Renderer: -renderer deferred

Load scene file from scenes folder (do not include path):
-scene [filename]

e.g. To load the scenes/canyon.txt scene: -scene canyon.txt

Credits for art content used:

The Fortress Forever (http://fortress-forever.com) artists. Particularly Sindre "decs" Grønvoll, Tommy "Blunkka" Blomqvist and Paul “MrBeefy” Painter.
Angel “R_Yell” Oliver for the canyon model & textures.
Simon “Nooba” Burford for the generator model & texture.
Hazel H. (http://www.hazelwhorley.com/textures.html) for the skybox textures.


