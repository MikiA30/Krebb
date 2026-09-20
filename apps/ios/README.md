# Krebb iOS app

Open [Krebb.xcodeproj](Krebb/Krebb.xcodeproj) in Xcode, select an iPhone simulator or your iPhone, then press Run.

```
Krebb/          Application source
KrebbTests/     Unit tests
KrebbUITests/   UI tests
```

The first app pass is a mock-mode SwiftUI prototype: it demonstrates an experimental temperature response, baseline flow, and experiment history before the ESP32 stream is connected. The real BLE adapter will decode the shared sensor schema in `shared/schemas/`.

The visual system uses Krebb’s supplied dark mark, brand red, and a coral-pink secondary signal color. The rings represent experimental response, sensor signal quality, and context completeness; they do not represent calories or clinical scores.
