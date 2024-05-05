name: "Ask a question, report a bug, request a feature, etc."
description: "Ask any question, discuss best practices, report a bug, request a feature."
body:
  
    attributes:
      value: |
        **Prerequisites:**
        - I have read [Contributing Guidelines](https://github.com/klonyyy/STMViewer/blob/devel/.github/CONTRIBUTING.md).
        ----
  - type: input
    id: specs_version
    attributes:
      label: "STMViewer Version/Branch:"
      value: "Version 0.X.Y, Branch: XXX (main/devel/etc.)"
      placeholder: "Version 1.XX, Branch: XXX (master/docking/etc.)"
    validations:
      required: true
  - type: input
    id: os
    attributes:
      label: "Operating system:"
      placeholder: "e.g. Windows 11, Ubuntu 22.04"
    validations:
      required: true
  - type: input
    id: probe
    attributes:
      label: "Debug probe:"
      placeholder: "e.g. ST-Link V2, ST-Link V3 MINIE, J-Link Ultra"
    validations:
      required: true
  - type: textarea
    id: issue_description
    attributes:
      label: "Details:"
      description: "Please describe the problem/enhancement in detail."
      value: |
        **My Issue/Question:**

        XXX _(please provide as much context as possible)_
    validations:
      required: true
  - type: textarea
    id: screenshots
    attributes:
      label: "Screenshots/Video:"
      description: "Attach screenshots or gif/videos to clarify the context. They often convey useful information that is omitted by the description."
      placeholder: "(Drag files here)"
    validations:
      required: false
  - type: textarea
    id: repro_code
    attributes:
      label: "Minimal, Complete and Verifiable Example code:"
      description: "Provide a step by step "
      value: |
        1. Run STMViewer
        2. Click X
        3. Select Y
        4. ...
    validations:
      required: false