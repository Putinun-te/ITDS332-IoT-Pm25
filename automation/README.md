# Automated Daily Forecast Run

This folder contains scripts to run the training and forecast notebook automatically every day on Windows.

## 1) Create environment file
Copy `.env.automation.example` to `.env.automation` at the workspace root, then fill your Firebase values.

## 2) Test one manual run
Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\automation\run_forecast_notebook.ps1
```

This executes `train_pm_colab_firebase.ipynb`, saves an executed notebook, and pushes forecast to Firebase (if enabled).

## 3) Register daily schedule
Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\automation\register_daily_task.ps1 -TaskName "PM25-Daily-Forecast" -StartTime "00:10"
```

## 4) Verify task status
Run:

```powershell
schtasks /Query /TN "PM25-Daily-Forecast" /V /FO LIST
```

If needed, trigger it immediately:

```powershell
schtasks /Run /TN "PM25-Daily-Forecast"
```
