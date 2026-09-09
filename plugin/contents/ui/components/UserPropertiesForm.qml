import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.5
import org.kde.kirigami 2.6 as Kirigami
import org.kde.plasma.components 3.0 as PlasmaComponents

import "../js/utils.mjs" as Utils

// Reusable user-properties form for any wallpaper whose project.json
// exposes `general.properties`.  Generates controls dynamically from
// property type metadata (slider, bool, combo, color, textinput, file,
// directory) and writes changes to the per-wallpaper config.
//
// Properties:
//   workshopId   — wallpaper workshop ID (empty string = no wallpaper,
//                  component auto-hides)
//   pyext        — FileHelper wrapper (Pyext instance) for reading
//                  project.json and writing config
//   projectJsonPath — absolute native path to the wallpaper's project.json
//                     (used to read general.properties metadata)
//   activeBindings — array of property names that are bound to shaders
//                    (empty = all shown as active; used to grey out unused)
//   configRev    — incremented externally to force re-read of saved values
//                  (e.g. after a config reset from another widget)
//
// Signals:
//   propChanged(key, value) — emitted when a property value is saved
//   propsReset() — emitted when all properties are reset to defaults
//
// Condition evaluation is NOT implemented in the form UI — the condition
// metadata field is preserved in property descriptors for future
// visibility support.  See WPUserProperties.hpp for the server-side
// evaluation logic (ResolveValue).

Item {
    id: root

    property string workshopId: ""
    property var pyext: null
    property string projectJsonPath: ""
    property var activeBindings: []
    property int configRev: 0

    signal propChanged(string key, var value)
    signal propsReset()

    implicitHeight: visible ? contentLayout.implicitHeight : 0
    implicitWidth: parent ? parent.width : 0
    visible: workshopId !== "" && propertyModel.length > 0

    // Internal state
    property var propertyModel: []
    property var propChanges: ({})
    property bool _loaded: false

    onWorkshopIdChanged: _loadProperties()
    onProjectJsonPathChanged: _loadProperties()
    onConfigRevChanged: _loadSavedOverrides()

    function _loadProperties() {
        if (!workshopId || !projectJsonPath || !pyext) {
            propertyModel = [];
            propChanges = {};
            _loaded = false;
            return;
        }

        pyext.read_wallpaper_properties(workshopId, projectJsonPath).then(function(props) {
            if (!Array.isArray(props)) { props = []; }
            propertyModel = props;
            propChanges = {};
            _loaded = true;
        });
    }

    function _loadSavedOverrides() {
        if (!workshopId || !pyext) return;
        _loadProperties();
    }

    function savePropChange(key, val) {
        if (!workshopId) return;

        var newChanges = JSON.parse(JSON.stringify(propChanges));
        newChanges[key] = val;
        propChanges = newChanges;

        var userPropsConfig = {};
        userPropsConfig['user_props'] = Object.assign({}, propFromConfig(), propChanges);
        pyext.write_wallpaper_config(workshopId, userPropsConfig);

        root.propChanged(key, val);
    }

    function getPropValue(key, defaultVal) {
        if (propChanges.hasOwnProperty(key)) return propChanges[key];
        return defaultVal;
    }

    function propFromConfig() { return {}; }

    function resetUserProps() {
        propChanges = {};
        var resetConfig = { 'user_props': {} };
        pyext.write_wallpaper_config(workshopId, resetConfig);
        _loadProperties();
        root.propsReset();
    }

    ColumnLayout {
        id: contentLayout
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 6

        Repeater {
            id: propertyRepeater
            model: root.propertyModel

            delegate: RowLayout {
                spacing: 8
                property bool isBound: {
                    var ab = root.activeBindings;
                    return !Array.isArray(ab) || ab.length === 0
                        || ab.indexOf(modelData.name) >= 0;
                }
                opacity: isBound ? 1.0 : 0.4

                Label {
                    Layout.preferredWidth: Math.max(120, implicitWidth)
                    text: Utils.formatPropertyLabel(modelData.text, modelData.name)
                    color: Kirigami.Theme.textColor
                    elide: Text.ElideRight
                }

                Loader {
                    id: propLoader
                    Layout.fillWidth: true
                    sourceComponent: root._controlForType(modelData.type)

                    onLoaded: {
                        var savedVal = root.getPropValue(modelData.name, null);
                        var defVal = savedVal !== null ? savedVal : modelData.value;

                        switch (modelData.type) {
                        case 'bool':
                            this.item.checked = Qt.binding(function() {
                                var sv = root.getPropValue(modelData.name, null);
                                return sv !== null ? Boolean(sv) : Boolean(modelData.value);
                            });
                            this.item.onToggled.connect(function() {
                                root.savePropChange(modelData.name, this.checked);
                            }.bind(this.item));
                            break;
                        case 'slider':
                            var isFloat = (modelData.min !== undefined && modelData.min % 1 !== 0) ||
                                        (modelData.max !== undefined && modelData.max % 1 !== 0) ||
                                        (modelData.value !== undefined && modelData.value % 1 !== 0);
                            if (isFloat) {
                                this.item.from = modelData.min !== undefined ? modelData.min : 0;
                                this.item.to = modelData.max !== undefined ? modelData.max : 1;
                                this.item.stepSize = modelData.step !== undefined ? modelData.step : 0.01;
                            } else {
                                this.item.from = modelData.min !== undefined ? Math.floor(modelData.min) : 0;
                                this.item.to = modelData.max !== undefined ? Math.floor(modelData.max) : 100;
                                this.item.stepSize = modelData.step !== undefined ? Math.floor(modelData.step) : 1;
                            }
                            this.item.value = Qt.binding(function() {
                                var sv = root.getPropValue(modelData.name, null);
                                return sv !== null ? Number(sv) : Number(modelData.value);
                            });
                            this.item.onValueChanged.connect(function() {
                                root.savePropChange(modelData.name, this.value);
                            }.bind(this.item));
                            break;
                        case 'color':
                            var c = defVal;
                            var colorStr = "#ffffff";
                            if (typeof c === 'string' && c.indexOf(' ') >= 0) {
                                var parts = c.split(' ').map(Number);
                                if (parts.length >= 3) {
                                    colorStr = Qt.rgba(parts[0], parts[1], parts[2], 1.0);
                                }
                            } else if (c && typeof c === 'string') {
                                colorStr = c;
                            }
                            this.item.def_val = colorStr;
                            this.item.colorValue = colorStr;
                            this.item.onColorPicked.connect(function(clr) {
                                root.savePropChange(modelData.name, clr.toString());
                            });
                            break;
                        case 'combo':
                            if (modelData.options) {
                                var comboModel = [];
                                var opts = modelData.options;
                                for (var i = 0; i < opts.length; i++) {
                                    var opt = opts[i];
                                    var label = String(opt.label || opt.value);
                                    comboModel.push({ text: label, value: opt.value });
                                }
                                this.item.model = comboModel;
                                this.item.currentIndex = Qt.binding(function() {
                                    var sv = root.getPropValue(modelData.name, null);
                                    var curVal = sv !== null ? sv : modelData.value;
                                    for (var j = 0; j < this.model.length; j++) {
                                        if (this.model[j].value === curVal) return j;
                                    }
                                    return 0;
                                }.bind(this.item));
                                this.item.onActivated.connect(function(idx) {
                                    root.savePropChange(modelData.name, this.model[idx].value);
                                }.bind(this.item));
                            }
                            break;
                        case 'textinput':
                        case 'file':
                        case 'directory':
                            this.item.text = Qt.binding(function() {
                                var sv = root.getPropValue(modelData.name, null);
                                return sv !== null ? String(sv) : String(modelData.value || "");
                            });
                            this.item.onTextChanged.connect(function() {
                                root.savePropChange(modelData.name, this.text);
                            }.bind(this.item));
                            break;
                        }
                    }
                }
            }
        }

        RowLayout {
            visible: root.propertyModel.length > 0
            Layout.topMargin: 4
            Item { Layout.fillWidth: true }
            Button {
                text: i18nc("@action:button reset user properties to defaults", "Reset to Defaults")
                onClicked: root.resetUserProps()
            }
        }
    }

    // ── Control factory ─────────────────────────────────────────────────
    function _controlForType(type) {
        switch(type) {
        case 'bool':       return switchComp;
        case 'slider':      return sliderComp;
        case 'color':       return colorComp;
        case 'combo':       return comboComp;
        case 'textinput':   return textInputComp;
        case 'file':
        case 'directory':   return textInputComp;
        }
        return null;
    }

    Component { id: switchComp;     Switch {} }
    Component { id: sliderComp;     SpinBox {} }
    Component { id: comboComp;      ComboBox { textRole: "text" } }
    Component { id: textInputComp;   TextField { Layout.fillWidth: true } }
    Component {
        id: colorComp
        RowLayout {
            spacing: 4
            property string colorValue: "#ffffff"
            property alias def_val: colorBtn.def_val
            signal colorPicked(color value)

            Rectangle {
                width: 24; height: 24; radius: 3
                color: parent.colorValue
                border.color: Kirigami.Theme.textColor
                border.width: 1
            }
            Button {
                id: colorBtn
                property color def_val: "#ffffff"
                text: i18nc("@action:button pick a color", "Pick\u2026")
                onClicked: colorDialog.open()
                Dialog {
                    id: colorDialog
                    title: i18nc("@title:window color picker", "Pick Color")
                    modal: true
                    implicitWidth: Kirigami.Units.gridUnit * 18
                    anchors.centerIn: Overlay.overlay
                    contentItem: ColumnLayout {
                        Label { text: i18nc("@info color picker hex entry", "Enter color as hex (e.g. #ff0000):") }
                        TextField { id: hexInput; placeholderText: "#ff0000" }
                        Label { text: i18nc("@info color picker preview", "Red/Green/Blue (0-255):") }
                        RowLayout {
                            SpinBox { id: rSpin; from: 0; to: 255; value: 255 }
                            SpinBox { id: gSpin; from: 0; to: 255 }
                            SpinBox { id: bSpin; from: 0; to: 255 }
                        }
                    }
                    standardButtons: Dialog.Ok | Dialog.Cancel
                    onAccepted: {
                        colorComp.colorValue = Qt.rgba(rSpin.value/255, gSpin.value/255, bSpin.value/255, 1.0);
                        colorComp.colorPicked(colorComp.colorValue);
                    }
                }
            }
        }
    }
}
