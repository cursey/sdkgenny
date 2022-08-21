Get-ChildItem -Path .\include,.\src,.\examples -Include *.hpp, *.cpp -Recurse | 
ForEach-Object {
    Write-Output $_.FullName
    &clang-format -i -style=file $_.FullName
}
