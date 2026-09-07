# TGP-FIELD

Fork personalizat al QField pentru TerraGIS Pro.

## Configurație stabilită

```text
APP_NAME=TGP-FIELD
APP_PACKAGE_ID=ro.tgp.field
IOS_APP_IDENTIFIER=ro.tgp.field
APP_ICON=tgp_field
APP_ICON_PATH=branding/tgp-field
APP_THEME_PATH=branding/tgp-field/theme.json
```

Codul original și modificările derivate rămân sub GPL-2.0-or-later. Numele și activele TGP-FIELD/TerraGIS Pro sunt branding separat și trebuie distribuite numai cu permisiunea titularului.

Vezi `docs/TGP_FIELD_ARCHITECTURE.md` pentru planul MEGA și fluxul de proiecte.
Vezi `docs/TGP_WINDOWS_DESKTOP.md` pentru interfața Windows, funcțiile GIS/GNSS
grupate după fluxurile de teren și configurarea installerului.

Exportul offline produce o arhivă `.tgpfield.zip` care conține proiectul QGIS,
GeoPackage-urile și atașamentele cu căi relative. Uploadul MEGA este reluabil și
rămâne în coada persistentă până la confirmarea transferului.

## Arhitectură TGP în cod

Codul se află în `src/tgp/` și este construit ca biblioteca statică `tgp_sync`.
Aplicația o expune în QML prin obiectul `tgpField`, cu proprietățile
`offlineExporter`, `syncEngine` și `cloudProvider`. Adaptorul MEGA oferă acum
formular de conectare, reluarea unei sesiuni criptate și deconectare. El rămâne
dezactivat în buildurile fără SDK-ul oficial și fără un App Key TGP-FIELD.

Parola MEGA nu este scrisă în fișiere, setări sau Git. După autentificare,
doar sesiunea returnată de SDK poate fi păstrată prin `QgsAuthManager`, care
folosește mecanismul securizat al platformei configurat deja de QField.

## Activarea conexiunii MEGA în installere

1. Creează o aplicație TGP-FIELD în portalul MEGA Developers și copiază App Key-ul.
2. În repository-ul GitHub, deschide **Settings → Secrets and variables → Actions**.
3. Creează secretul de repository `TGP_MEGA_APP_KEY` și introdu App Key-ul, nu
   parola contului MEGA.
4. Rulează din nou workflow-urile **Windows** și **Android**.

Workflow-urile descarcă SDK-ul oficial MEGA `v9.16.1` și activează autentificarea
numai când secretul există. Dacă lipsește, installerele se construiesc în
continuare, dar interfața va raporta că SDK-ul MEGA nu este inclus. Conectarea și
reluarea sesiunii sunt implementate; uploadul arhivei de proiect este o etapă
separată și nu este încă funcțional.
