# Third-party references and dependencies

This package contains original application source and procedural preview artwork written for Anz. No source files or assets from Pip-KeyPet are bundled. The product idea and general portable-C/LVGL approach were informed by that reference.

- **Pip-KeyPet**, NovikovVisuals, MIT: https://github.com/NovikovVisuals/Pip-KeyPet . If future development copies its source/assets, preserve its applicable copyright and license notices.
- **TuyaOpen**, Tuya, Apache-2.0: https://github.com/tuya/TuyaOpen . Required external SDK; not bundled. Initialization, configuration and build integration were written against its published APIs/examples. Preserve upstream notices in a final distribution of SDK code and components.
- **LVGL**, MIT: https://github.com/lvgl/lvgl . Included through TuyaOpen, not bundled here. Preserve its license when distributing a binary containing LVGL.
- **System fonts**: browser preview relies on fonts already installed on the viewer's computer. No font files are redistributed.

Audit transitive SDK components and any later added fonts, icons, audio and sprite packs before commercial distribution. This package does not make ownership or trademark-clearance claims over the working name Pebble.
