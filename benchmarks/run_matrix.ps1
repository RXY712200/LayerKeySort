param(
    [Parameter(Mandatory = $true)][string]$Executable,
    [Parameter(Mandatory = $true)][string]$Version,
    [Parameter(Mandatory = $true)][string]$Output,
    [ValidateSet('Common', 'Current')][string]$Profile = 'Common',
    [int]$Warmups = 1,
    [int]$Repetitions = 7,
    [string]$Seed = '0x91A30D47'
)

$ErrorActionPreference = 'Stop'
if (-not (Test-Path -LiteralPath $Executable)) {
    throw "Executable not found: $Executable"
}
$cases = @(
    @('tree', 'ascending', 1000), @('tree', 'descending', 1000),
    @('tree', 'random', 1000), @('tree', 'equal', 1000),
    @('tree', 'duplicates', 1000), @('tree', 'alternating', 1000),
    @('tree', 'ascending', 10000), @('tree', 'descending', 10000),
    @('tree', 'random', 10000), @('tree', 'equal', 4000),
    @('tree', 'duplicates', 4000), @('tree', 'alternating', 4000),
    @('tree', 'two', 4000), @('tree', 'eight', 4000),
    @('tree', 'sixtyfour', 4000), @('tree', 'middle', 4000),
    @('group', 'random', 10000), @('group', 'duplicates', 10000),
    @('group', 'ascending', 10000), @('group', 'descending', 10000),
    @('group_merge', 'duplicates', 10000),
    @('batch', 'duplicates', 10000),
    @('batch_merge', 'duplicates', 10000),
    @('sort', 'random', 10000), @('sort', 'ascending', 10000),
    @('sort', 'descending', 10000), @('sort', 'duplicates', 10000),
    @('sort', 'equal', 10000), @('qsort', 'random', 10000),
    @('qsort', 'duplicates', 10000)
)
if ($Profile -eq 'Current') {
    $cases += @(
        @('tree', 'hotspot', 4000),
        @('tree', 'hotspot', 10000),
        @('tree', 'ascending', 100000), @('tree', 'descending', 100000),
        @('tree', 'random', 100000), @('tree', 'equal', 100000),
        @('tree', 'duplicates', 100000),
        @('group', 'random', 100000), @('batch', 'duplicates', 100000),
        @('batch_merge', 'duplicates', 100000),
        @('sort', 'random', 100000), @('qsort', 'random', 100000),
        @('mut_remove_random', 'random', 10000),
        @('mut_remove_all', 'random', 10000),
        @('mut_rekey_random', 'random', 10000),
        @('mut_rekey_hotspot', 'random', 10000),
        @('mut_mixed', 'random', 10000),
        @('path_format', 'ascending', 1),
        @('path_parse', 'ascending', 1),
        @('key_format', 'ascending', 1),
        @('key_parse', 'ascending', 1),
        @('path_format', 'ascending', 64),
        @('path_parse', 'ascending', 64),
        @('key_format', 'ascending', 64),
        @('key_parse', 'ascending', 64),
        @('path_format', 'ascending', 1000),
        @('path_parse', 'ascending', 1000),
        @('key_format', 'ascending', 1000),
        @('key_parse', 'ascending', 1000),
        @('path_format', 'ascending', 10000),
        @('path_parse', 'ascending', 10000),
        @('key_format', 'ascending', 10000),
        @('key_parse', 'ascending', 10000)
    )
}

$rows = [System.Collections.Generic.List[string]]::new()
$rows.Add('version,operation,distribution,n,warmups,repetitions,median_ms,min_ms,max_ms,output_bytes')
foreach ($case in $cases) {
    $line = & $Executable $case[0] $case[1] $case[2] $Warmups $Repetitions $Seed
    if ($LASTEXITCODE -ne 0 -or $line.Count -ne 1 -or
        $line -notmatch '^[a-z_]+,[a-z]+,[0-9]+,[0-9]+,[0-9]+,') {
        throw "Benchmark failed: $($case -join ' '): $line"
    }
    $rows.Add("$Version,$line")
    Write-Output "$Version,$line"
}
$rows | Set-Content -LiteralPath $Output -Encoding ascii
