# Krebb iOS app

Open [Krebb.xcodeproj](Krebb/Krebb.xcodeproj) in Xcode, select an iPhone simulator or your iPhone, then press Run.

```
Krebb/Krebb/          Application source
Krebb/KrebbTests/     Unit tests
Krebb/KrebbUITests/   UI tests
```

The app is a sample-data SwiftUI prototype. Today shows an interactive temperature curve relative to baseline; Check previews the baseline and observation stages; Journal opens an explicitly labeled example session. Sample checks retain their stage across tabs but do not collect or persist real measurements. The BLE adapter will later decode the shared sensor schema in `shared/schemas/`.

The interface uses native `TabView`, a `NavigationStack` per tab, large navigation titles, system sheets, and Liquid Glass buttons on iOS 26. The coral temperature curve and quiet dark surfaces provide Krebb’s visual identity without activity rings or invented response percentages. Chart values are sample temperature changes, not metabolic scores.

The app icon and toolbar mark are rendered directly from the supplied SVG geometry with `swift scripts/render-brand.swift "$PWD"` from the repository root. Both assets use a full opaque square; the system or view applies the corner mask. This avoids the extra white canvas introduced by Quick Look thumbnails.

UI tests cover title collapse on scroll, native detail/back navigation, the sensor sheet, and retaining the sample check stage across tab changes.
