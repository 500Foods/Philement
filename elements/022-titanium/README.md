# Titanium: High-Performance Video Streaming Component

## Overview

Titanium is a lightweight, high-performance video streaming component designed for resource-constrained devices like Raspberry Pi-based 3D printers. It provides efficient camera capture, hardware-accelerated encoding, and WebRTC streaming capabilities while maintaining minimal CPU and memory footprint.

**Key Features:**

- Direct V4L2 camera access with hardware acceleration
- Custom H.264 encoding optimized for low latency
- WebRTC P2P streaming with Mirage proxy integration
- 3D printer-optimized resource management
- Browser-agnostic WebAssembly video plugins

## Architecture

### Core Components

**Camera Subsystem:**

- Direct V4L2 kernel interface for camera capture
- Hardware-accelerated encoding using VideoCore (RPi)
- Ring buffer for smooth frame handling
- Real-time thread scheduling for consistent performance

**Streaming Engine:**

- Custom WebRTC implementation (minimal signaling overhead)
- SRTP encryption for secure P2P communication
- Adaptive bitrate based on network conditions
- Fallback to MJPEG for compatibility

**Resource Manager:**

- CPU affinity assignment to protect print operations
- Memory limits with emergency reserves
- Automatic quality reduction under load
- Print-priority pause/resume functionality

### Integration with Hydrogen/Mirage

Titanium integrates seamlessly with Hydrogen's ecosystem:

```structure
3D Printer (Hydrogen + Titanium)
    ↓ WebRTC Signaling
Mirage Proxy Server
    ↓ P2P Video Stream
Remote Client (WebAssembly Plugin)
```

**Hydrogen Integration Points:**

- Queue system for stream coordination
- Configuration management for camera settings
- WebSocket extensions for video control
- Database logging of stream sessions

## Performance Specifications

### Resource Usage Targets

| Resolution | Frame Rate | CPU Usage | Memory | Latency |
|------------|------------|-----------|--------|---------|
| 720p | 30fps | 10-15% | 80MB | <50ms |
| 1080p | 30fps | 15-25% | 120MB | <70ms |
| 1080p | 60fps | 25-35% | 150MB | <100ms |

### Hardware Compatibility

**Raspberry Pi 4:**

- H.264 hardware encoding up to 1080p30
- USB camera support via V4L2
- CPU usage: 15-25% for 720p streaming

**Raspberry Pi 5:**

- Improved VideoCore VII performance
- Better thermal management for sustained streaming
- CPU usage: 10-20% for 720p streaming

## Implementation Strategy

### Phase 1: MJPEG Foundation

- Direct V4L2 camera capture
- Hardware MJPEG encoding
- HTTP streaming for local access
- Basic configuration and controls

### Phase 2: H.264 Acceleration

- VideoCore H.264 encoder integration
- RTP packetization for network transport
- Quality and bitrate controls
- Performance monitoring

### Phase 3: WebRTC Integration

- Minimal WebRTC signaling implementation
- STUN/TURN server integration
- P2P connection establishment
- Encryption and security

### Phase 4: Mirage Proxy Integration

- Signaling through Mirage tunnels
- Authentication and authorization
- Multi-device stream management
- Remote access controls

## 3D Printer Optimization

### Print Protection Features

**Automatic Resource Management:**

```c
// Monitor system resources during printing
if (print_active && cpu_usage > 60) {
    titanium_stream_reduce_quality();
}

if (thermal_anomaly_detected) {
    titanium_stream_pause();
}
```

**Priority-Based Streaming:**

- **High Priority**: Print monitoring (always active)
- **Medium Priority**: Remote access when requested
- **Low Priority**: Background streaming (suspend during critical operations)

### Camera Placement Considerations

**3D Printer Use Cases:**

- **Build Chamber**: Wide-angle view of entire print area
- **Nozzle Camera**: Close-up view of print head and first layer
- **Multi-Camera**: Different angles for comprehensive monitoring

**Lighting Optimization:**

- IR illumination for low-light conditions
- Automatic exposure adjustment for different materials
- Motion-triggered quality boosts

## WebAssembly Video Plugin

### Existing Libraries and Frameworks

Several mature WebRTC and video streaming libraries can accelerate Titanium plugin development:

**PeerJS (Simple WebRTC Wrapper):**

```javascript
import Peer from 'peerjs';

const peer = new Peer();
peer.call('titanium-camera-123', localStream);
```

- Simple API for WebRTC connections
- Built-in signaling (can be adapted for Mirage)

**Simple-Peer (Lightweight WebRTC):**

```javascript
import SimplePeer from 'simple-peer';

const peer = new SimplePeer({initiator: true});
peer.signal(mirageSignalingData);
```

- Minimal WebRTC implementation
- Manual signaling control (ideal for Mirage integration)

**WebRTC.rs (Rust → WebAssembly):**

```rust
use webrtc::peer_connection::RTCPeerConnection;
let peer = RTCPeerConnection::new().await?;
```

- High-performance Rust WebRTC
- Compiles to WebAssembly
- Memory-safe and efficient

**Video.js with WebRTC Plugin:**

```html
<video-js>
  <source src="webrtc://titanium-stream" type="application/webrtc">
</video-js>
```

- Mature video player framework
- WebRTC streaming support
- Extensive customization options

### Plugin Architecture

**Titanium WebRTC Component:**

```javascript
class TitaniumVideoPlayer {
  constructor(options) {
    this.cameraId = options.cameraId;
    this.mirageServer = options.mirageServer;
    this.quality = options.quality || '720p30';
    this.library = options.library || 'simple-peer'; // PeerJS, Simple-Peer, etc.
  }

  async connect() {
    // WebRTC peer connection establishment
    // Signaling through Mirage proxy
    // Direct P2P video streaming
  }

  setQuality(profile) {
    // Adaptive bitrate adjustment
  }
}
```

**Web Component Wrapper:**

```javascript
class TitaniumVideo extends HTMLElement {
  connectedCallback() {
    this.player = new TitaniumVideoPlayer({
      cameraId: this.getAttribute('camera-id'),
      mirageServer: this.getAttribute('mirage-server')
    });
  }
}
customElements.define('titanium-video', TitaniumVideo);
```

### Browser Compatibility

**Supported Browsers:**

- Chrome/Chromium (full WebRTC support)
- Firefox (full WebRTC support)
- Safari (WebRTC with limitations)
- Mobile browsers (adaptive quality)

**Progressive Enhancement:**

- WebRTC P2P for modern browsers
- HLS/DASH fallback for older browsers
- MJPEG streaming for basic compatibility

## Configuration

### Camera Configuration

```json
{
  "titanium": {
    "enabled": true,
    "cameras": [
      {
        "id": "chamber",
        "device": "/dev/video0",
        "resolution": "1280x720",
        "fps": 30,
        "codec": "h264",
        "bitrate": "2000k",
        "controls": {
          "brightness": 50,
          "contrast": 60,
          "saturation": 40
        }
      }
    ],
    "streaming": {
      "default_quality": "720p30",
      "max_viewers": 5,
      "adaptive_bitrate": true,
      "print_protection": true
    }
  }
}
```

### Mirage Integration

```json
{
  "mirage": {
    "titanium_streams": {
      "enabled": true,
      "signaling_port": 8443,
      "stun_servers": ["stun:stun.l.google.com:19302"],
      "turn_servers": []
    }
  }
}
```

## Development Roadmap

### Immediate Tasks (Week 1-4)

- [ ] V4L2 camera capture implementation
- [ ] Basic MJPEG streaming
- [ ] Hardware encoding integration
- [ ] Configuration system
- [ ] Basic WebRTC signaling

### Medium Term (Month 2-3)

- [ ] Full WebRTC P2P streaming
- [ ] Mirage proxy integration
- [ ] WebAssembly plugin development
- [ ] Performance optimization
- [ ] 3D printer testing

### Long Term (Month 4-6)

- [ ] Multi-camera support
- [ ] Advanced video analytics
- [ ] Cloud storage integration
- [ ] Mobile app development
- [ ] Enterprise features

## Testing Strategy

### Unit Testing

- Camera capture functionality
- Encoding performance
- Memory usage validation
- Error handling scenarios

### Integration Testing

- WebRTC connection establishment
- Mirage proxy compatibility
- Multi-client streaming
- Network condition simulation

### 3D Printer Validation

- Print quality impact assessment
- Thermal management testing
- Long-duration reliability
- Recovery from interruptions

## Performance Benchmarks

### Baseline Performance (RPi 4)

- **720p30 MJPEG**: 8% CPU, 45MB RAM
- **720p30 H.264**: 18% CPU, 85MB RAM
- **1080p30 H.264**: 28% CPU, 110MB RAM

### Target Performance (RPi 5)

- **720p30 H.264**: 12% CPU, 75MB RAM
- **1080p30 H.264**: 20% CPU, 95MB RAM
- **1080p60 H.264**: 30% CPU, 125MB RAM

## Security Considerations

- **Stream Encryption**: SRTP for all video streams
- **Access Control**: Device-level and user-level permissions
- **DDoS Protection**: Rate limiting and connection monitoring
- **Privacy**: No video storage by default, client-side control

## Future Enhancements

- **AI Analytics**: Object detection for print monitoring
- **Cloud Integration**: Video storage and processing
- **Multi-Format Support**: Additional codecs (AV1, VP9)
- **Edge Computing**: Local video analysis on device
- **5G Integration**: High-bandwidth mobile streaming

---

**Note:** While Titanium is developed as a separate component for modularity, its code will be primarily embedded within Hydrogen for optimal performance and tight integration.