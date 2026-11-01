$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$project=Get-Content (Join-Path $root 'X360Browser.vcxproj') -Raw
$units=[regex]::Matches($project,'    <ClCompile Include="vendor\\bearssl\\.*?</ClCompile>') | ForEach-Object {$_.Value.Replace('Include="vendor\','Include="..\vendor\')}
$xml=@'
<Project DefaultTargets="Build" ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemGroup Label="ProjectConfigurations"><ProjectConfiguration Include="Release|Win32"><Configuration>Release</Configuration><Platform>Win32</Platform></ProjectConfiguration></ItemGroup>
  <PropertyGroup Label="Globals"><ProjectGuid>{D84D6CF4-EBEA-4B32-9A61-EBC52184867C}</ProjectGuid><Keyword>Win32Proj</Keyword></PropertyGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.Default.props" />
  <PropertyGroup Label="Configuration"><ConfigurationType>Application</ConfigurationType><PlatformToolset>v100</PlatformToolset><CharacterSet>MultiByte</CharacterSet></PropertyGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.props" />
  <PropertyGroup><OutDir>$(ProjectDir)</OutDir><IntDir>$(ProjectDir)native-host\</IntDir><TargetName>NativeHost</TargetName></PropertyGroup>
  <ItemDefinitionGroup>
    <ClCompile><Optimization>MaxSpeed</Optimization><RuntimeLibrary>MultiThreaded</RuntimeLibrary><ExceptionHandling>Sync</ExceptionHandling><WarningLevel>Level3</WarningLevel><PreprocessorDefinitions>WIN32;_CONSOLE;X360BROWSER_HOST_TEST;_CRT_SECURE_NO_WARNINGS</PreprocessorDefinitions><AdditionalIncludeDirectories>$(ProjectDir)..\src;$(ProjectDir)..\vendor\orbita;$(ProjectDir)..\vendor\bearssl;$(ProjectDir)..\vendor\bearssl\inc</AdditionalIncludeDirectories></ClCompile>
    <Link><AdditionalDependencies>ws2_32.lib;advapi32.lib;%(AdditionalDependencies)</AdditionalDependencies><SubSystem>Console</SubSystem></Link>
  </ItemDefinitionGroup>
  <ItemGroup>
    <ClCompile Include="..\tests\native_checks.cpp" />
    <ClCompile Include="..\vendor\orbita\xui_gfx.c"><CompileAs>CompileAsCpp</CompileAs></ClCompile>
    <ClCompile Include="..\vendor\orbita\xui_text.c"><CompileAs>CompileAsCpp</CompileAs></ClCompile>
UNITS
  </ItemGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" />
</Project>
'@
$xml=$xml.Replace('UNITS',($units -join "`n"))
$target=Join-Path $root 'obj\NativeHost.vcxproj'
[IO.File]::WriteAllText($target,$xml)
& "$env:WINDIR\Microsoft.NET\Framework\v4.0.30319\MSBuild.exe" $target '/p:Configuration=Release' '/p:Platform=Win32' /v:minimal /nologo
if($LASTEXITCODE -ne 0){throw 'Native host test build failed.'}
Push-Location $root
try {& (Join-Path $root 'obj\NativeHost.exe');if($LASTEXITCODE -ne 0){throw 'Native checks failed.'}} finally {Pop-Location}
