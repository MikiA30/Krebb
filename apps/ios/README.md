# Krebb iOS app

Open [Krebb.xcodeproj](Krebb/Krebb.xcodeproj) in Xcode, select an iPhone simulator or your iPhone, then press Run.

```
Krebb/Krebb/          Application source
Krebb/KrebbTests/     Unit tests
Krebb/KrebbUITests/   UI tests
```

The app records local sessions from either simulation or the Krebb One BLE stream. Check handles scanning, explicit BLE connection, live packet recording, baseline, observation, Health context attachment, and saving. Journal shows saved sessions, response features, and a debug export for the future pipeline.

The interface uses native `TabView`, a `NavigationStack` per tab, large navigation titles, system sheets, and Liquid Glass buttons on iOS 26. The coral temperature curve and quiet dark surfaces provide Krebb’s visual identity without activity rings or invented response percentages. Chart values are sample temperature changes, not metabolic scores.

The app icon and toolbar mark are rendered directly from the supplied SVG geometry with `swift scripts/render-brand.swift "$PWD"` from the repository root. Both assets use a full opaque square; the system or view applies the corner mask. This avoids the extra white canvas introduced by Quick Look thumbnails.

UI tests cover title collapse on scroll, native detail/back navigation, the sensor sheet, and retaining the sample check stage across tab changes.
