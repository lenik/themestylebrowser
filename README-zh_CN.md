# Theme Style Browser (tsb)

按 **主题/风格** 与 **包** 组织的图标库图形浏览器。

## 布局

库根目录形如：

```
<library-root>/
├── .themestyles          # 主题/风格目录列表（如 flex/regular、pixel）
├── flex/
│   ├── regular/
│   │   ├── computer/     # 包目录
│   │   │   ├── mouse.png
│   │   │   └── keyboard.png
│   │   └── travel/
│   └── line/
├── freehand/
│   ├── color/
│   └── regular/
└── pixel/
```

`.themestyles` 格式（忽略注释与空行）：

```
# 示例
flex/regular
flex/line
freehand/color
pixel
```

## 用法

- **运行：** `themestylebrowser [OPTIONS] [<librarydir>]`
- **文件 → 打开库：** 选择含 `.themestyles` 的库根目录。
- **左侧：** 包树（可展开/折叠）。节点标签：`名称 (文件数/项目数)`。
- **右侧：** 文件列表 — 名称、数量、大小，以及每个 themestyle 一列（缩略图或 `-`）。
- **查看菜单：** 按名称（默认）、数量或大小排序。
- **列标题：** 左键点击名称/数量/大小排序；右键点击名称可切换 themestyle 列可见性。
- **双击** 文件名 → 项目属性；双击图标单元格 → 图像属性（路径/代码片段、复制）。

## 构建

- **依赖：** meson、ninja、pkg-config、libbas-c、libbas-cpp、libbas-ui、wxWidgets（GTK3；兼容 3.0 / 3.2）、asciidoctor。
- **构建：**
  ```bash
  meson setup build && meson compile -C build
  ```
- **运行：** `./build/themestylebrowser`，或安装后：`themestylebrowser`。

## Debian

```bash
dpkg-buildpackage -us -uc -b
```

## 作者

Lenik

## 许可证

AGPL-3.0-or-later，附加限制（反 AI 声明）：

**禁止将本软件或其源代码用于训练、微调或评估机器学习或人工智能系统。**
