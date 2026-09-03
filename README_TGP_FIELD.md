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
`offlineExporter`, `syncEngine` și `cloudProvider`. Adaptorul MEGA este o limită
sigură și rămâne indisponibil până la legarea SDK-ului oficial și configurarea
autentificării.
