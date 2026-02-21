const {
  Document, Packer, Paragraph, TextRun, Table, TableRow, TableCell,
  HeadingLevel, AlignmentType, BorderStyle, WidthType, ShadingType,
  LevelFormat
} = require('docx');
const fs = require('fs');

const border = { style: BorderStyle.SINGLE, size: 1, color: "DDDDDD" };
const borders = { top: border, bottom: border, left: border, right: border };

function heading1(text) {
  return new Paragraph({
    heading: HeadingLevel.HEADING_1,
    spacing: { before: 320, after: 120 },
    children: [new TextRun({ text, bold: true, size: 28, color: "1F3864" })]
  });
}

function heading2(text) {
  return new Paragraph({
    heading: HeadingLevel.HEADING_2,
    spacing: { before: 200, after: 80 },
    children: [new TextRun({ text, bold: true, size: 24, color: "2E75B6" })]
  });
}

function body(text) {
  return new Paragraph({
    spacing: { before: 60, after: 60 },
    children: [new TextRun({ text, size: 22 })]
  });
}

function label(text) {
  return new TextRun({ text, bold: true, size: 22 });
}

function normal(text) {
  return new TextRun({ text, size: 22 });
}

function codeBlock(text) {
  return new Paragraph({
    spacing: { before: 60, after: 60 },
    indent: { left: 480 },
    children: [new TextRun({ text, font: "Courier New", size: 18, color: "C0392B" })]
  });
}

function divider() {
  return new Paragraph({
    spacing: { before: 160, after: 160 },
    border: { bottom: { style: BorderStyle.SINGLE, size: 4, color: "CCCCCC" } },
    children: []
  });
}

function bugTable(num, bug, cause, fix) {
  return new Table({
    width: { size: 9360, type: WidthType.DXA },
    columnWidths: [1440, 7920],
    rows: [
      // Header row
      new TableRow({
        children: [
          new TableCell({
            borders,
            width: { size: 9360, type: WidthType.DXA },
            columnSpan: 2,
            shading: { fill: "1F3864", type: ShadingType.CLEAR },
            margins: { top: 100, bottom: 100, left: 160, right: 160 },
            children: [new Paragraph({
              children: [new TextRun({ text: `Bug #${num}: ${bug}`, bold: true, size: 22, color: "FFFFFF" })]
            })]
          })
        ]
      }),
      // Cause row
      new TableRow({
        children: [
          new TableCell({
            borders,
            width: { size: 1440, type: WidthType.DXA },
            shading: { fill: "F2F2F2", type: ShadingType.CLEAR },
            margins: { top: 80, bottom: 80, left: 160, right: 160 },
            children: [new Paragraph({ children: [new TextRun({ text: "Cause", bold: true, size: 20, color: "2E75B6" })] })]
          }),
          new TableCell({
            borders,
            width: { size: 7920, type: WidthType.DXA },
            margins: { top: 80, bottom: 80, left: 160, right: 160 },
            children: cause.map(line => new Paragraph({ spacing: { before: 40, after: 40 }, children: [new TextRun({ text: line, size: 20 })] }))
          })
        ]
      }),
      // Fix row
      new TableRow({
        children: [
          new TableCell({
            borders,
            width: { size: 1440, type: WidthType.DXA },
            shading: { fill: "F2F2F2", type: ShadingType.CLEAR },
            margins: { top: 80, bottom: 80, left: 160, right: 160 },
            children: [new Paragraph({ children: [new TextRun({ text: "Fix", bold: true, size: 20, color: "2E75B6" })] })]
          }),
          new TableCell({
            borders,
            width: { size: 7920, type: WidthType.DXA },
            margins: { top: 80, bottom: 80, left: 160, right: 160 },
            children: fix.map(line => new Paragraph({ spacing: { before: 40, after: 40 }, children: [new TextRun({ text: line, size: 20 })] }))
          })
        ]
      }),
    ]
  });
}

function spacer() {
  return new Paragraph({ children: [], spacing: { before: 160, after: 0 } });
}

const doc = new Document({
  styles: {
    default: { document: { run: { font: "Arial", size: 22 } } },
    paragraphStyles: [
      { id: "Heading1", name: "Heading 1", basedOn: "Normal", next: "Normal", quickFormat: true,
        run: { size: 28, bold: true, font: "Arial", color: "1F3864" },
        paragraph: { spacing: { before: 320, after: 120 }, outlineLevel: 0 } },
      { id: "Heading2", name: "Heading 2", basedOn: "Normal", next: "Normal", quickFormat: true,
        run: { size: 24, bold: true, font: "Arial", color: "2E75B6" },
        paragraph: { spacing: { before: 200, after: 80 }, outlineLevel: 1 } },
    ]
  },
  sections: [{
    properties: {
      page: {
        size: { width: 12240, height: 15840 },
        margin: { top: 1440, right: 1440, bottom: 1440, left: 1440 }
      }
    },
    children: [
      // Title
      new Paragraph({
        alignment: AlignmentType.CENTER,
        spacing: { before: 0, after: 80 },
        children: [new TextRun({ text: "RenderX Vulkan — Bug Log", bold: true, size: 40, color: "1F3864" })]
      }),
      new Paragraph({
        alignment: AlignmentType.CENTER,
        spacing: { before: 0, after: 320 },
        children: [new TextRun({ text: "Errors encountered and how they were fixed", size: 22, color: "666666", italics: true })]
      }),
      divider(),

      // Bug 1
      spacer(),
      bugTable(
        1,
        "Staging Allocator Buffer Overflow",
        [
          "The chunk-based staging allocator had three problems at once:",
          "1. std::vector<StagingChunk> m_AllChunks reallocated on push_back, invalidating the m_CurrentChunk raw pointer.",
          "2. Chunks were copied by value between m_AllChunks, m_InFlightChunks, and m_FreeChunks. When recycling a free chunk the code searched m_AllChunks by VkBuffer handle but never re-added it, so it returned a stale entry with a full currentOffset.",
          "3. StagingChunk::allocate() logged the overflow but continued anyway, returning an out-of-bounds offset.",
          "",
          "Validation errors seen:",
          "VUID-vkCmdCopyBufferToImage: trying to copy past end of VkBuffer",
          "VUID-vkCmdCopyBuffer-srcOffset: srcOffset (75498496) > size of srcBuffer (67108864)"
        ],
        [
          "Rewrote the staging allocator as a single-use batch uploader (VulkanLoadTimeStagingUploader):",
          "- All upload data is appended to a CPU-side std::vector<uint8_t> first.",
          "- On flush(), one staging VkBuffer is created at the exact size needed.",
          "- All barriers and copies are recorded in a single command buffer and submitted once.",
          "- The staging buffer is destroyed immediately after the fence signals.",
          "",
          "Result: one staging buffer, one GPU submit, zero overflow bugs, memory freed right after load."
        ]
      ),
      spacer(),

      // Bug 2
      bugTable(
        2,
        "Shadow Map Layout Transition Missing",
        [
          "The shadow map texture was never transitioned out of UNDEFINED before being used as a depth attachment.",
          "The barrier call existed but was either not reached on the first frame or had the wrong old layout."
        ],
        [
          "Added an explicit UndefinedToDepthStencil barrier for the shadow map before the shadow pass.",
          "Used UNDEFINED as oldLayout so the barrier is always valid regardless of frame number."
        ]
      ),
      spacer(),

      // Bug 3
      bugTable(
        3,
        "Depth Buffer Barrier Commented Out + Wrong Array Size",
        [
          "In ModelRenderer::forwardPass(), the depth buffer barrier was commented out:",
          "    TextureBarrier barriers[1] = {",
          "        TextureBarrier::PresentToColorAttachment(swapTex),",
          "        // TextureBarrier::UndefinedToDepthStencil(m_DepthBuffer),  <- commented out",
          "    };",
          "Two issues: depth buffer was never transitioned from UNDEFINED, and the array was declared size 1 with 2 initializers (undefined behaviour)."
        ],
        [
          "Uncommented the depth barrier and fixed the array size:",
          "    TextureBarrier barriers[2] = {",
          "        TextureBarrier::PresentToColorAttachment(swapTex),",
          "        TextureBarrier::UndefinedToDepthStencil(m_DepthBuffer),",
          "    };",
          "    cmd->Barrier(nullptr, 0, nullptr, 0, barriers, 2);",
          "",
          "Since the depth attachment uses LoadOp::CLEAR every frame, UNDEFINED as old layout is always correct."
        ]
      ),
      spacer(),

      // Bug 4
      bugTable(
        4,
        "Swapchain Image Layout Wrong on First Frame",
        [
          "The forward pass used TextureBarrier::PresentToColorAttachment() to transition the swapchain image before rendering.",
          "That preset sets oldLayout = PRESENT.",
          "",
          "On the very first frame (and any frame using an image that has never been presented), the actual layout is UNDEFINED, not PRESENT.",
          "The driver ignores a barrier whose oldLayout does not match the real layout, so the transition never happened.",
          "beginRendering() then tried to use the image as a color attachment while it was still UNDEFINED.",
          "",
          "Validation error: VUID-vkCmdDraw-None-09600: expects VkImage to be in VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, instead current layout is VK_IMAGE_LAYOUT_UNDEFINED"
        ],
        [
          "Changed the swapchain barrier to always use UNDEFINED as oldLayout:",
          "    TextureBarrier(swapTex,",
          "        TextureLayout::UNDEFINED,          // always safe, discards previous contents",
          "        TextureLayout::COLOR_ATTACHMENT,",
          "        PipelineStage::TOP_OF_PIPE, AccessFlags::NONE,",
          "        PipelineStage::COLOR_ATTACHMENT_OUTPUT, AccessFlags::COLOR_ATTACHMENT_WRITE);",
          "",
          "UNDEFINED as oldLayout is always valid when you are about to clear the image anyway.",
          "This works correctly on frame 0 (image truly UNDEFINED) and all subsequent frames (previous contents discarded, which matches LoadOp::CLEAR)."
        ]
      ),
      spacer(),
      divider(),

      new Paragraph({
        alignment: AlignmentType.CENTER,
        spacing: { before: 200, after: 0 },
        children: [new TextRun({ text: "4 bugs fixed — renderer stable", size: 20, color: "888888", italics: true })]
      }),
    ]
  }]
});

Packer.toBuffer(doc).then(buffer => {
  fs.writeFileSync('/mnt/user-data/outputs/RenderX_BugLog.docx', buffer);
  console.log('Done');
});