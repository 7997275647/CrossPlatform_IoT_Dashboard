import QtQuick

// Line chart of the samples stored in a SensorChannel (one line per series).
Item {
    id: root

    required property var channel
    property int windowSize: 1024
    property bool showStats: false
    property color textColor: "#808080"
    property color gridColor: "#40808080"
    property var seriesColors: ["#e4572e", "#29b36b", "#3a86ff"]

    implicitHeight: 180

    function requestRepaint() { canvas.requestPaint() }

    onWindowSizeChanged: canvas.requestPaint()
    onShowStatsChanged: canvas.requestPaint()
    onTextColorChanged: canvas.requestPaint()
    onWidthChanged: canvas.requestPaint()
    onHeightChanged: canvas.requestPaint()

    Connections {
        target: root.channel
        function onSamplesChanged() { canvas.requestPaint() }
    }

    Canvas {
        id: canvas
        anchors.fill: parent
        renderTarget: Canvas.Image

        onPaint: {
            const ctx = getContext("2d")
            ctx.reset()

            const ch = root.channel
            const left = 56, right = 10, top = 8, bottom = 22
            const w = width - left - right
            const h = height - top - bottom
            if (w <= 0 || h <= 0)
                return

            const names = ch.seriesNames
            const series = []
            let yMin = Infinity, yMax = -Infinity, xMin = Infinity, xMax = -Infinity
            for (let s = 0; s < names.length; ++s) {
                const pts = ch.samples(s, root.windowSize)
                series.push(pts)
                for (let i = 0; i < pts.length; ++i) {
                    yMin = Math.min(yMin, pts[i].y)
                    yMax = Math.max(yMax, pts[i].y)
                }
                if (pts.length > 0) {
                    xMin = Math.min(xMin, pts[0].x)
                    xMax = Math.max(xMax, pts[pts.length - 1].x)
                }
            }

            ctx.font = "11px sans-serif"
            ctx.fillStyle = root.textColor
            if (!isFinite(yMin)) {
                ctx.textAlign = "center"
                ctx.fillText(qsTr("Waiting for data on %1").arg(ch.topic), left + w / 2, top + h / 2)
                return
            }
            if (yMax - yMin < 1e-9) { yMin -= 1; yMax += 1 }
            const pad = (yMax - yMin) * 0.08
            yMin -= pad; yMax += pad
            if (xMax - xMin < 1) xMax = xMin + 1000

            const px = x => left + (x - xMin) / (xMax - xMin) * w
            const py = y => top + h - (y - yMin) / (yMax - yMin) * h

            // grid + y labels
            ctx.strokeStyle = root.gridColor
            ctx.lineWidth = 1
            ctx.textAlign = "right"
            ctx.textBaseline = "middle"
            for (let g = 0; g <= 4; ++g) {
                const y = top + h * g / 4
                ctx.beginPath(); ctx.moveTo(left, y); ctx.lineTo(left + w, y); ctx.stroke()
                ctx.fillText((yMax - (yMax - yMin) * g / 4).toFixed(1), left - 6, y)
            }

            // x labels
            ctx.textBaseline = "top"
            for (let g = 0; g <= 3; ++g) {
                const x = left + w * g / 3
                ctx.textAlign = g === 0 ? "left" : (g === 3 ? "right" : "center")
                ctx.fillText(Qt.formatTime(new Date(xMin + (xMax - xMin) * g / 3), "hh:mm:ss"), x, top + h + 6)
            }

            // min / max / average lines (first series only to stay readable)
            if (root.showStats && series[0].length > 0) {
                const stats = [ch.minimum[0], ch.maximum[0], ch.average[0]]
                ctx.setLineDash([4, 4])
                ctx.strokeStyle = root.textColor
                for (let k = 0; k < stats.length; ++k) {
                    const y = py(stats[k])
                    ctx.beginPath(); ctx.moveTo(left, y); ctx.lineTo(left + w, y); ctx.stroke()
                }
                ctx.setLineDash([])
            }

            // data
            ctx.lineWidth = 1.5
            ctx.lineJoin = "round"
            for (let s = 0; s < series.length; ++s) {
                const pts = series[s]
                ctx.strokeStyle = root.seriesColors[s % root.seriesColors.length]
                ctx.beginPath()
                for (let i = 0; i < pts.length; ++i) {
                    if (i === 0) ctx.moveTo(px(pts[i].x), py(pts[i].y))
                    else ctx.lineTo(px(pts[i].x), py(pts[i].y))
                }
                ctx.stroke()
            }
        }
    }
}
