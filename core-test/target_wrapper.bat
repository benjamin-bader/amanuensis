@echo off
SetLocal EnableDelayedExpansion
(set PATH=C:\Qt\5.8\msvc2015\bin;!PATH!)
if defined QT_PLUGIN_PATH (
    set QT_PLUGIN_PATH=C:\Qt\5.8\msvc2015\plugins;!QT_PLUGIN_PATH!
) else (
    set QT_PLUGIN_PATH=C:\Qt\5.8\msvc2015\plugins
)
%*
EndLocal
