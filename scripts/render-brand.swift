// Rasterize the supplied krebb-app-icon.svg geometry without Quick Look's thumbnail canvas.
// Usage: swift scripts/render-brand.swift <repository root>
import AppKit

let root = URL(fileURLWithPath: CommandLine.arguments[1])
let size = 1024
let context = CGContext(data: nil, width: size, height: size, bitsPerComponent: 8,
    bytesPerRow: size * 4, space: CGColorSpace(name: CGColorSpace.sRGB)!,
    bitmapInfo: CGImageAlphaInfo.noneSkipLast.rawValue)!
let black = CGColor(red: 17/255, green: 17/255, blue: 19/255, alpha: 1)
let white = CGColor(red: 244/255, green: 242/255, blue: 239/255, alpha: 1)
let red = CGColor(red: 200/255, green: 30/255, blue: 20/255, alpha: 1)
context.setFillColor(black)
context.fill(CGRect(x: 0, y: 0, width: size, height: size))
context.translateBy(x: 0, y: CGFloat(size))
context.scaleBy(x: CGFloat(size)/200, y: -CGFloat(size)/200)
context.translateBy(x: 24, y: 24)
context.scaleBy(x: 0.76, y: 0.76)
context.setStrokeColor(white)
context.setLineWidth(13)
context.strokeEllipse(in: CGRect(x: 26, y: 26, width: 148, height: 148))

func circle(_ x: CGFloat, _ y: CGFloat, _ r: CGFloat, _ color: CGColor) {
    context.setFillColor(color)
    context.fillEllipse(in: CGRect(x: x-r, y: y-r, width: 2*r, height: 2*r))
}
circle(100, 26, 16, red)
for (x, y) in [(152.3,47.7), (174.0,100.0), (152.3,152.3), (100.0,174.0), (47.7,152.3), (26.0,100.0), (47.7,47.7)] {
    circle(x, y, 11, white)
}
context.setFillColor(red)
context.move(to: CGPoint(x: 100, y: 58))
context.addLine(to: CGPoint(x: 136.4, y: 121))
context.addLine(to: CGPoint(x: 63.6, y: 121))
context.closePath()
context.fillPath()
context.setStrokeColor(black)
context.setLineWidth(5)
context.setLineCap(.round)
context.setLineJoin(.round)
context.move(to: CGPoint(x: 82.5, y: 104.2))
for (x, y) in [(91.9,104.2), (99.2,89.4), (105.6,108.7), (111.2,96.6), (118.2,96.6)] {
    context.addLine(to: CGPoint(x: x, y: y))
}
context.strokePath()
let bitmap = NSBitmapImageRep(cgImage: context.makeImage()!)
let png = bitmap.representation(using: .png, properties: [:])!
for path in ["KrebbMark.imageset/krebb-mark.png", "AppIcon.appiconset/krebb-app-icon.png"] {
    try png.write(to: root.appendingPathComponent("apps/ios/Krebb/Krebb/Assets.xcassets/" + path))
}
