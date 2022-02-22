Get-ChildItem -Path .\include,.\examples -Include *.hpp, *.cpp -Recurse | 
ForEach-Object {
    Write-Output $_.FullName
    &clang-format -i -style=file $_.FullName
}
