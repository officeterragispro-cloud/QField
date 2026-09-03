# TGP-FIELD pentru Windows

## Fluxul de lucru

1. Utilizatorul deschide un proiect QGIS existent sau creează unul nou.
2. Ecranul desktop grupează proiectele recente, instrumentele de teren, hărțile și sincronizarea.
3. Datele locale pot fi deschise din GeoPackage, Shapefile, GeoJSON, KML/KMZ și MBTiles.
4. Proiectul folosește în continuare formularele, relațiile, stilurile și regulile definite în QGIS.
5. Instrumentele GNSS/RTK, NTRIP, medierea poziției, trackingul, stakeout și COGO sunt furnizate de motorul QField.
6. „Create offline package” salvează proiectul curent și detectează automat toate sursele `.gpkg` încărcate.
7. Dacă există un director `attachments` lângă proiect, acesta este inclus în snapshot.
8. Arhiva `.tgpfield.zip` intră în coada persistentă pentru MEGA.

## Compatibilitate inspirată de fluxurile SW Maps

TGP-FIELD nu copiază interfața sau activele SW Maps. Funcțiile echivalente sunt expuse prin componentele QGIS/QField deja licențiate și prin interfața proprie TGP.

| Flux utilizator | Implementare TGP-FIELD |
|---|---|
| Date vectoriale offline | GeoPackage, Shapefile, GeoJSON și KML prin furnizorii QGIS |
| Hărți raster offline | MBTiles și proiecte QGIS pregătite pentru teren |
| Servicii online | WMS, WMTS, XYZ și TMS configurate în proiectul QGIS |
| GNSS de precizie | receptoare Bluetooth/serial, NTRIP și înălțime antenă |
| Ridicare puncte | formulare QGIS, mediere poziție și controlul preciziei |
| Ridicare trasee | tracking în strat liniar |
| Trasare | navigare/stakeout către punct sau entitate |
| Schimb de date | proiect QGIS + GeoPackage + atașamente într-o arhivă verificată |

## Build Windows

Workflow-ul `.github/workflows/windows.yml` produce:

- `TGP-FIELD-x64.msi` pentru publicări și rulări manuale;
- `TGP-FIELD-arm64.msi` pentru Windows ARM;
- arhive ZIP în verificările de tip pull request.

Installerul folosește numele, furnizorul, pictograma și GUID-ul de upgrade proprii TGP-FIELD. Pentru distribuție publică trebuie configurat separat un certificat de semnare de cod deținut de TerraGIS Pro.

## Limită curentă

Adaptorul MEGA este separat de interfață și coadă. Exportul local este funcțional, dar uploadul remote rămâne blocat explicit până la legarea SDK-ului oficial MEGA și configurarea autentificării securizate. Aplicația nu raportează un transfer ca reușit înainte de confirmarea furnizorului.
