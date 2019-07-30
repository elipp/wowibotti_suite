@echo off

FXC /Fo vs.o /T vs_3_0 vs.hlsl || goto :error
FXC /Fo ps.o /T ps_3_0 ps.hlsl || goto :error
FXC /Fo quad_ps.o /T ps_3_0 quad_ps.hlsl || goto :error
FXC /Fo quad_vs.o /T vs_3_0 quad_vs.hlsl || goto :error

FXC /Fo gradient_vs.o /T vs_3_0 gradient_vs.hlsl || goto :error
FXC /Fo gradient_ps.o /T ps_3_0 gradient_ps.hlsl || goto :error
FXC /Fo rgradient_ps.o /T ps_3_0 rgradient_ps.hlsl || goto :error


echo Compilation successful!
goto :EOF

:error

echo Compilation failed lulz
exit /b %errorlevel%
