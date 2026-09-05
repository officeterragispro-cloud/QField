import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import org.qfield.core
import org.qfield.gui

Item {
  id: root

  property var recentProjectsModel
  property int currentSection: 0
  readonly property color navy: "#0A2B50"
  readonly property color deepNavy: "#061822"
  readonly property color green: "#145A1C"
  readonly property color actionGreen: "#43A047"
  readonly property color blue: "#2A6497"
  readonly property color ice: "#EBF3FD"
  readonly property bool hasOpenProject: qgisProject && qgisProject.fileName !== ""

  signal showLocalDataPicker
  signal showProjectCreationScreen
  signal showSettings
  signal returnToMap

  Rectangle {
    anchors.fill: parent
    color: ice
  }

  RowLayout {
    anchors.fill: parent
    spacing: 0

    Rectangle {
      Layout.fillHeight: true
      Layout.preferredWidth: 252
      color: deepNavy

      ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
          Layout.fillWidth: true
          Layout.margins: 22
          spacing: 13

          Image {
            Layout.preferredWidth: 54
            Layout.preferredHeight: 54
            source: "qrc:/images/app_logo.svg"
            fillMode: Image.PreserveAspectFit
            smooth: true
          }

          ColumnLayout {
            spacing: 0
            Label {
              text: "TGP-FIELD"
              color: "white"
              font.pixelSize: 20
              font.bold: true
            }
            Label {
              text: "TerraGIS Pro"
              color: "#ACD1EC"
              font.pixelSize: 12
            }
          }
        }

        Rectangle {
          Layout.fillWidth: true
          Layout.preferredHeight: 1
          color: "#1C4568"
        }

        Repeater {
          model: [
            {
              "label": qsTr("Projects"),
              "detail": qsTr("QGIS workspaces")
            },
            {
              "label": qsTr("Field tools"),
              "detail": qsTr("Survey and navigation")
            },
            {
              "label": qsTr("Maps & layers"),
              "detail": qsTr("Local and online data")
            },
            {
              "label": qsTr("Offline & MEGA"),
              "detail": qsTr("Exports and transfer queue")
            }
          ]

          delegate: Rectangle {
            required property int index
            required property var modelData
            Layout.fillWidth: true
            Layout.preferredHeight: 66
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            radius: 10
            color: root.currentSection === index ? "#17466E" : "transparent"

            Rectangle {
              visible: root.currentSection === parent.index
              anchors.left: parent.left
              anchors.verticalCenter: parent.verticalCenter
              width: 4
              height: 34
              radius: 2
              color: actionGreen
            }

            Column {
              anchors.left: parent.left
              anchors.leftMargin: 20
              anchors.verticalCenter: parent.verticalCenter
              spacing: 3
              Label {
                text: parent.parent.modelData.label
                color: "white"
                font.pixelSize: 14
                font.bold: root.currentSection === parent.parent.index
              }
              Label {
                text: parent.parent.modelData.detail
                color: "#ACD1EC"
                font.pixelSize: 11
              }
            }

            MouseArea {
              anchors.fill: parent
              cursorShape: Qt.PointingHandCursor
              onClicked: root.currentSection = parent.index
            }
          }
        }

        Item {
          Layout.fillHeight: true
        }

        Rectangle {
          Layout.fillWidth: true
          Layout.margins: 14
          Layout.preferredHeight: 122
          radius: 12
          color: "#123B61"
          Column {
            anchors.fill: parent
            anchors.margins: 14
            spacing: 5
            Label {
              text: qsTr("Cloud provider")
              color: "#ACD1EC"
              font.pixelSize: 11
            }
            Row {
              spacing: 7
              Rectangle {
                width: 9
                height: 9
                radius: 5
                color: tgpField.cloudProvider.ready ? actionGreen : "#CA9315"
                anchors.verticalCenter: parent.verticalCenter
              }
              Label {
                text: tgpField.cloudProvider.ready ? qsTr("MEGA connected") : qsTr("MEGA not configured")
                color: "white"
                font.pixelSize: 13
                font.bold: true
              }
            }
            Label {
              text: qsTr("%n transfer(s) pending", "", tgpField.offlineExporter.pendingCount)
              color: "#ACD1EC"
              font.pixelSize: 11
            }
            Button {
              width: parent.width
              height: 28
              flat: true
              text: tgpField.cloudProvider.ready ? qsTr("Account settings") : qsTr("Configure MEGA")
              palette.buttonText: "white"
              onClicked: megaLoginDialog.open()
            }
          }
        }

        Button {
          Layout.fillWidth: true
          Layout.margins: 14
          Layout.topMargin: 0
          text: qsTr("Settings")
          flat: true
          palette.buttonText: "white"
          onClicked: root.showSettings()
        }
      }
    }

    ColumnLayout {
      Layout.fillWidth: true
      Layout.fillHeight: true
      spacing: 0

      Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 76
        color: "white"
        border.color: "#D4E1ED"

        RowLayout {
          anchors.fill: parent
          anchors.leftMargin: 30
          anchors.rightMargin: 30
          ColumnLayout {
            spacing: 2
            Label {
              text: [qsTr("My projects"), qsTr("Field tools"), qsTr("Maps & layers"), qsTr("Offline & MEGA")][root.currentSection]
              color: navy
              font.pixelSize: 21
              font.bold: true
            }
            Label {
              text: qsTr("Desktop workspace · Windows")
              color: "#527086"
              font.pixelSize: 12
            }
          }
          Item {
            Layout.fillWidth: true
          }
          Rectangle {
            Layout.preferredWidth: statusText.width + 26
            Layout.preferredHeight: 34
            radius: 17
            color: hasOpenProject ? "#E2F3E4" : "#E8F0F7"
            Label {
              id: statusText
              anchors.centerIn: parent
              text: hasOpenProject ? qsTr("Project open") : qsTr("No project open")
              color: hasOpenProject ? green : blue
              font.bold: true
              font.pixelSize: 12
            }
          }
          Button {
            visible: hasOpenProject
            text: qsTr("Return to map")
            highlighted: true
            onClicked: root.returnToMap()
          }
        }
      }

      StackLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        currentIndex: root.currentSection

        ScrollView {
          clip: true
          contentWidth: availableWidth
          ColumnLayout {
            width: parent.width - 60
            x: 30
            y: 30
            spacing: 20

            RowLayout {
              Layout.fillWidth: true
              ColumnLayout {
                spacing: 5
                Label {
                  text: qsTr("Workspaces prepared for the field")
                  color: navy
                  font.pixelSize: 28
                  font.bold: true
                }
                Label {
                  text: qsTr("Open a QGIS project, import local GIS data or create a new project.")
                  color: "#527086"
                  font.pixelSize: 14
                }
              }
              Item {
                Layout.fillWidth: true
              }
              Button {
                text: qsTr("Open local project")
                onClicked: root.showLocalDataPicker()
              }
              Button {
                text: qsTr("New project")
                highlighted: true
                onClicked: root.showProjectCreationScreen()
              }
            }

            RowLayout {
              Layout.fillWidth: true
              spacing: 14
              TgpMetricCard {
                Layout.fillWidth: true
                title: qsTr("Recent projects")
                value: recentProjects.count.toString()
                accent: root.blue
              }
              TgpMetricCard {
                Layout.fillWidth: true
                title: qsTr("Offline exports")
                value: tgpField.offlineExporter.jobs.length.toString()
                accent: root.green
              }
              TgpMetricCard {
                Layout.fillWidth: true
                title: qsTr("Pending transfers")
                value: tgpField.offlineExporter.pendingCount.toString()
                accent: "#CA9315"
              }
            }

            Label {
              text: qsTr("Recently opened")
              color: navy
              font.pixelSize: 17
              font.bold: true
              Layout.topMargin: 4
            }

            GridView {
              id: recentProjects
              Layout.fillWidth: true
              Layout.preferredHeight: Math.max(230, contentHeight)
              cellWidth: 278
              cellHeight: 150
              interactive: false
              model: root.recentProjectsModel
              delegate: Rectangle {
                property string projectPath: ProjectPath
                property string projectTitle: ProjectTitle
                property int projectType: ProjectType
                width: 262
                height: 132
                radius: 14
                color: "white"
                border.color: "#D4E1ED"
                layer.enabled: projectMouse.containsMouse

                Rectangle {
                  anchors.left: parent.left
                  anchors.top: parent.top
                  anchors.bottom: parent.bottom
                  width: 6
                  radius: 3
                  color: projectType === 1 ? root.blue : root.green
                }
                Column {
                  anchors.fill: parent
                  anchors.margins: 18
                  anchors.leftMargin: 22
                  spacing: 8
                  Label {
                    text: projectType === 1 ? qsTr("CLOUD PROJECT") : qsTr("LOCAL PROJECT")
                    color: projectType === 1 ? root.blue : root.green
                    font.bold: true
                    font.pixelSize: 10
                  }
                  Label {
                    width: parent.width
                    text: projectTitle
                    color: root.deepNavy
                    font.bold: true
                    font.pixelSize: 16
                    elide: Text.ElideRight
                  }
                  Label {
                    width: parent.width
                    text: projectPath
                    color: "#6A8192"
                    font.pixelSize: 10
                    elide: Text.ElideMiddle
                  }
                  Label {
                    text: qsTr("Open project  →")
                    color: root.blue
                    font.bold: true
                    font.pixelSize: 12
                  }
                }
                MouseArea {
                  id: projectMouse
                  anchors.fill: parent
                  hoverEnabled: true
                  cursorShape: Qt.PointingHandCursor
                  onClicked: iface.loadFile(parent.projectPath, parent.projectTitle)
                }
              }
            }

            Rectangle {
              visible: recentProjects.count === 0
              Layout.fillWidth: true
              Layout.preferredHeight: 150
              radius: 14
              color: "white"
              border.color: "#D4E1ED"
              Column {
                anchors.centerIn: parent
                spacing: 8
                Label {
                  anchors.horizontalCenter: parent.horizontalCenter
                  text: qsTr("No recent projects")
                  color: navy
                  font.bold: true
                  font.pixelSize: 15
                }
                Label {
                  text: qsTr("Open a .qgz/.qgs file or create the first project.")
                  color: "#6A8192"
                }
              }
            }
          }
        }

        TgpFeaturePage {
          title: qsTr("Survey and field tools")
          subtitle: qsTr("SW Maps-style access to the surveying capabilities already available in the QField engine.")
          entries: [
            {
              "title": qsTr("GNSS / RTK"),
              "text": qsTr("Bluetooth or serial receivers, NTRIP corrections and antenna height."),
              "tag": qsTr("EXISTING ENGINE"),
              "action": "settings"
            },
            {
              "title": qsTr("Position averaging"),
              "text": qsTr("Collect multiple observations for a more stable point."),
              "tag": qsTr("FIELD"),
              "action": "map"
            },
            {
              "title": qsTr("Stakeout"),
              "text": qsTr("Navigate to a point or feature with distance and bearing."),
              "tag": qsTr("NAVIGATION"),
              "action": "map"
            },
            {
              "title": qsTr("Track recording"),
              "text": qsTr("Record walking or vehicle tracks directly into a line layer."),
              "tag": qsTr("OFFLINE"),
              "action": "map"
            },
            {
              "title": qsTr("Geometry capture"),
              "text": qsTr("Points, lines and polygons with snapping and forms."),
              "tag": qsTr("QGIS FORMS"),
              "action": "map"
            },
            {
              "title": qsTr("COGO & measurement"),
              "text": qsTr("Distances, areas, bearings and construction geometry."),
              "tag": qsTr("TOOLS"),
              "action": "map"
            }
          ]
          onActionRequested: action => root.handleFeatureAction(action)
        }

        TgpFeaturePage {
          title: qsTr("Maps and interoperable layers")
          subtitle: qsTr("Open field data directly and keep the original coordinate reference system.")
          entries: [
            {
              "title": "GeoPackage",
              "text": qsTr("Vector layers, attributes and styles in a single offline file."),
              "tag": ".GPKG",
              "action": "local"
            },
            {
              "title": "Shapefile / GeoJSON",
              "text": qsTr("Import common exchange formats for inspection or editing."),
              "tag": ".SHP · .GEOJSON",
              "action": "local"
            },
            {
              "title": "KML / KMZ",
              "text": qsTr("Open overlays and data exchanged with field teams."),
              "tag": ".KML · .KMZ",
              "action": "local"
            },
            {
              "title": "MBTiles",
              "text": qsTr("Use raster basemaps completely offline."),
              "tag": ".MBTILES",
              "action": "local"
            },
            {
              "title": "WMS / WMTS",
              "text": qsTr("Connect institutional map services from a QGIS project."),
              "tag": qsTr("ONLINE"),
              "action": "settings"
            },
            {
              "title": "XYZ / TMS",
              "text": qsTr("Use configurable web tiles and prepared offline caches."),
              "tag": qsTr("BASEMAP"),
              "action": "settings"
            }
          ]
          onActionRequested: action => root.handleFeatureAction(action)
        }

        ScrollView {
          clip: true
          contentWidth: availableWidth
          ColumnLayout {
            width: parent.width - 60
            x: 30
            y: 30
            spacing: 18
            Label {
              text: qsTr("Offline export and MEGA transfer queue")
              color: navy
              font.pixelSize: 28
              font.bold: true
            }
            Label {
              text: qsTr("Create an immutable archive containing the QGIS project, detected GeoPackages and attachments.")
              color: "#527086"
              font.pixelSize: 14
            }

            Rectangle {
              Layout.fillWidth: true
              Layout.preferredHeight: 158
              radius: 15
              color: "white"
              border.color: "#D4E1ED"
              RowLayout {
                anchors.fill: parent
                anchors.margins: 22
                ColumnLayout {
                  spacing: 6
                  Label {
                    text: hasOpenProject ? QfFileUtils.fileName(qgisProject.fileName, false) : qsTr("No active project")
                    color: navy
                    font.pixelSize: 18
                    font.bold: true
                  }
                  Label {
                    text: hasOpenProject ? qsTr("GeoPackage layers are detected automatically from the project.") : qsTr("Open a QGIS project before creating an export.")
                    color: "#527086"
                  }
                  Label {
                    text: qsTr("Destination: /TGP-FIELD/default/<project>/exports/")
                    color: blue
                    font.pixelSize: 11
                  }
                }
                Item {
                  Layout.fillWidth: true
                }
                Button {
                  text: qsTr("Create offline package")
                  highlighted: true
                  enabled: hasOpenProject
                  onClicked: {
                    const projectId = QfFileUtils.fileName(qgisProject.fileName, false);
                    tgpField.offlineExporter.enqueueCurrentProject(qgisProject, projectId, "default", true);
                  }
                }
              }
            }

            RowLayout {
              Layout.fillWidth: true
              Label {
                text: qsTr("Transfer queue")
                color: navy
                font.pixelSize: 17
                font.bold: true
              }
              Item {
                Layout.fillWidth: true
              }
              Button {
                text: tgpField.cloudProvider.ready ? qsTr("Disconnect MEGA") : qsTr("Connect MEGA")
                onClicked: tgpField.cloudProvider.ready ? tgpField.cloudProvider.logout() : megaLoginDialog.open()
              }
              Button {
                text: qsTr("Retry pending")
                enabled: tgpField.offlineExporter.pendingCount > 0
                onClicked: tgpField.syncEngine.synchronize()
              }
              Button {
                text: qsTr("Open export folder")
                onClicked: Qt.openUrlExternally("file:///" + tgpField.offlineExporter.storageDirectory)
              }
            }

            ListView {
              Layout.fillWidth: true
              Layout.preferredHeight: Math.max(170, contentHeight)
              spacing: 10
              interactive: false
              model: tgpField.offlineExporter.jobs
              delegate: Rectangle {
                required property var modelData
                width: ListView.view.width
                height: 82
                radius: 12
                color: "white"
                border.color: "#D4E1ED"
                RowLayout {
                  anchors.fill: parent
                  anchors.margins: 14
                  Rectangle {
                    Layout.preferredWidth: 10
                    Layout.preferredHeight: 10
                    radius: 5
                    color: modelData.state === "complete" ? actionGreen : modelData.state === "failed" ? "#B3261E" : "#CA9315"
                  }
                  ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 3
                    Label {
                      text: modelData.projectId
                      color: navy
                      font.bold: true
                    }
                    Label {
                      Layout.fillWidth: true
                      text: modelData.remotePath
                      color: "#6A8192"
                      font.pixelSize: 10
                      elide: Text.ElideMiddle
                    }
                  }
                  Label {
                    text: root.stateLabel(modelData.state)
                    color: blue
                    font.bold: true
                    font.pixelSize: 12
                  }
                  Button {
                    visible: modelData.state === "failed"
                    text: qsTr("Retry")
                    onClicked: tgpField.syncEngine.retry(modelData.jobId)
                  }
                }
              }
            }

            Rectangle {
              visible: tgpField.offlineExporter.jobs.length === 0
              Layout.fillWidth: true
              Layout.preferredHeight: 140
              radius: 14
              color: "white"
              border.color: "#D4E1ED"
              Label {
                anchors.centerIn: parent
                text: qsTr("The export queue is empty.")
                color: "#6A8192"
              }
            }
          }
        }
      }
    }
  }

  Dialog {
    id: megaLoginDialog
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(460, root.width - 40)
    modal: true
    focus: true
    title: qsTr("MEGA account")
    standardButtons: Dialog.NoButton

    onOpened: {
      megaEmail.text = tgpField.cloudProvider.accountEmail;
      megaPassword.text = "";
      megaEmail.forceActiveFocus();
    }

    ColumnLayout {
      width: parent.width
      spacing: 14

      Label {
        Layout.fillWidth: true
        text: tgpField.cloudProvider.ready ? qsTr("Connected as %1").arg(tgpField.cloudProvider.accountEmail) : qsTr("Connect TGP-FIELD to your MEGA account.")
        color: root.navy
        font.pixelSize: 17
        font.bold: true
        wrapMode: Text.WordWrap
      }

      Rectangle {
        visible: !tgpField.cloudProvider.sdkAvailable
        Layout.fillWidth: true
        Layout.preferredHeight: sdkMessage.implicitHeight + 24
        radius: 8
        color: "#FFF4D6"
        border.color: "#E2B84B"
        Label {
          id: sdkMessage
          anchors.fill: parent
          anchors.margins: 12
          text: qsTr("This build does not include the MEGA SDK yet. Account details cannot be submitted.")
          color: "#6E4C00"
          wrapMode: Text.WordWrap
        }
      }

      TextField {
        id: megaEmail
        Layout.fillWidth: true
        visible: !tgpField.cloudProvider.ready
        placeholderText: qsTr("MEGA email")
        inputMethodHints: Qt.ImhEmailCharactersOnly | Qt.ImhNoAutoUppercase
      }

      TextField {
        id: megaPassword
        Layout.fillWidth: true
        visible: !tgpField.cloudProvider.ready
        placeholderText: qsTr("Password")
        echoMode: TextInput.Password
        inputMethodHints: Qt.ImhSensitiveData | Qt.ImhNoPredictiveText
        onAccepted: megaLoginDialog.attemptLogin()
      }

      CheckBox {
        id: rememberMegaSession
        visible: !tgpField.cloudProvider.ready
        checked: true
        text: qsTr("Keep the encrypted session on this device")
      }

      Label {
        Layout.fillWidth: true
        text: tgpField.cloudProvider.statusMessage
        color: "#527086"
        wrapMode: Text.WordWrap
      }

      RowLayout {
        Layout.fillWidth: true
        Item {
          Layout.fillWidth: true
        }
        Button {
          text: qsTr("Close")
          onClicked: megaLoginDialog.close()
        }
        Button {
          visible: tgpField.cloudProvider.ready
          text: qsTr("Disconnect")
          onClicked: {
            tgpField.cloudProvider.logout();
            megaLoginDialog.close();
          }
        }
        Button {
          visible: !tgpField.cloudProvider.ready
          text: tgpField.cloudProvider.authenticating ? qsTr("Connecting…") : qsTr("Connect")
          highlighted: true
          enabled: tgpField.cloudProvider.sdkAvailable && !tgpField.cloudProvider.authenticating && megaEmail.text.trim() !== "" && megaPassword.text !== ""
          onClicked: megaLoginDialog.attemptLogin()
        }
      }
    }

    function attemptLogin() {
      if (!tgpField.cloudProvider.sdkAvailable || tgpField.cloudProvider.authenticating)
        return;
      const password = megaPassword.text;
      megaPassword.text = "";
      tgpField.cloudProvider.login(megaEmail.text.trim(), password, rememberMegaSession.checked);
    }
  }

  Connections {
    target: tgpField.offlineExporter
    function onExportQueued(jobId, archive) {
      mainWindow.displayToast(qsTr("Offline package created: %1").arg(QfFileUtils.fileName(QfUrlUtils.toLocalFile(archive))));
    }
    function onExportStateChanged(jobId, state, message) {
      mainWindow.displayToast(message);
    }
  }

  Connections {
    target: tgpField.cloudProvider
    function onAuthenticationFinished(success, message) {
      megaPassword.text = "";
      mainWindow.displayToast(message);
      if (success)
        megaLoginDialog.close();
    }
  }

  function handleFeatureAction(action) {
    if (action === "local") {
      showLocalDataPicker();
    } else if (action === "settings") {
      showSettings();
    } else if (action === "map") {
      if (hasOpenProject)
        returnToMap();
      else
        mainWindow.displayToast(qsTr("Open a project to use this field tool."));
    }
  }

  function stateLabel(state) {
    const labels = {
      "preparing": qsTr("Preparing"),
      "waiting_for_network": qsTr("Waiting for network"),
      "uploading": qsTr("Uploading"),
      "verifying": qsTr("Verifying"),
      "complete": qsTr("Complete"),
      "failed": qsTr("Failed"),
      "cancelled": qsTr("Cancelled")
    };
    return labels[state] || state;
  }
}
