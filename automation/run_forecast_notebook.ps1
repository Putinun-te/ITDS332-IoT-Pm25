param(
    [string]$WorkspaceRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,
    [string]$NotebookPath = "train_pm_colab_firebase.ipynb",
    [string]$OutputNotebook = "train_pm_colab_firebase.executed.ipynb",
    [int]$TimeoutSeconds = 1800
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$pythonExe = Join-Path $WorkspaceRoot ".venv\Scripts\python.exe"
if (-not (Test-Path $pythonExe)) {
    throw "Python executable not found at $pythonExe. Create/select .venv first."
}

$envFile = Join-Path $WorkspaceRoot ".env.automation"
if (Test-Path $envFile) {
    Get-Content -Path $envFile | ForEach-Object {
        $line = $_.Trim()
        if (-not $line -or $line.StartsWith("#")) {
            return
        }

        $parts = $line -split "=", 2
        if ($parts.Count -ne 2) {
            return
        }

        $name = $parts[0].Trim()
        $value = $parts[1].Trim()

        if (
            ($value.StartsWith('"') -and $value.EndsWith('"')) -or
            ($value.StartsWith("'") -and $value.EndsWith("'"))
        ) {
            $value = $value.Substring(1, $value.Length - 2)
        }

        [Environment]::SetEnvironmentVariable($name, $value, "Process")
    }

    Write-Host "Loaded environment from $envFile"
} else {
    Write-Host "No .env.automation file found. Using defaults from notebook cells."
}

$notebookFullPath = Join-Path $WorkspaceRoot $NotebookPath
if (-not (Test-Path $notebookFullPath)) {
    throw "Notebook not found: $notebookFullPath"
}

Push-Location $WorkspaceRoot
try {
    Write-Host "Ensuring Python dependencies are installed..."
    & $pythonExe -m pip install --disable-pip-version-check pandas numpy scikit-learn requests joblib matplotlib nbformat nbclient jupyter ipykernel
    if ($LASTEXITCODE -ne 0) {
        throw "Failed to install Python dependencies."
    }

    Write-Host "Executing notebook: $notebookFullPath"
    & $pythonExe -m jupyter nbconvert --to notebook --execute $notebookFullPath --output $OutputNotebook --ExecutePreprocessor.timeout=$TimeoutSeconds --ExecutePreprocessor.kernel_name=python3

    if ($LASTEXITCODE -ne 0) {
        throw "Notebook execution failed."
    }

    Write-Host "Notebook execution completed successfully."
    Write-Host "Output notebook: $OutputNotebook"
} finally {
    Pop-Location
}
