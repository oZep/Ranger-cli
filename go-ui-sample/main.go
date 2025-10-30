package main

import (
	"bufio"
	"encoding/json"
	"fmt"
	"net"
	"io"
	"os"
	"sync"
	"time"

	tea "github.com/charmbracelet/bubbletea"
	"github.com/charmbracelet/lipgloss"
)

type PacketEvent struct {
	Type    string `json:"type"`
	ID      int    `json:"id"`
	Dir     string `json:"dir"`
	TS      int64  `json:"ts"`
	Src     string `json:"src"`
	Dst     string `json:"dst"`
	Len     int    `json:"len"`
	Payload string `json:"payload"`
}

type clientModel struct {
	mu       sync.Mutex
	packets  []PacketEvent
	selected int

	conn     net.Conn
	ch       chan PacketEvent
	connected bool
	connecting bool

	host string
	port int
}

func newClientModel() *clientModel {
	m := &clientModel{
		ch:			make(chan PacketEvent, 256),
		host:		"127.0.0.1",
		port:		9001,
		selected: 	0,
		connected: 	false,
	}
	return m
}

func (m *clientModel) connect() {
	m.mu.Lock()
	if m.connecting || m.connected {
		m.mu.Unlock()
		return
	}
	m.connecting = true
	m.mu.Unlock()

	go func() {
		addr := fmt.Sprintf("%s:%d", m.host, m.port)
		conn, err := net.DialTimeout("tcp", addr, 2*time.Second)
		m.mu.Lock()
		if err != nil { // failure
			m.connecting = false
			m.mu.Unlock()
			return
		}
		m.conn = conn
		m.connected = true
		m.connecting = false
		m.mu.Unlock()

		reader := bufio.NewReader(conn)
		for {
			line, err := reader.ReadString('\n')
			if err != nill {
				if err == io.EOF {
					// connection closed
				}
				m.mu.Lock()
				if m.conn != nil {
					m.conn.Close()
				}
				m.connected = false
				m.mu.Unlock()
				conn.Close()
				break
			}
			var ev PacketEvent
			if err := json.Unmarshal([]byte(line), &ev); err != nil {
				// ignore parse errors for now; TODO: surface parse errors to UI
				continue
			}
			select {
			case m.ch <- ev:
			default:
				// channel full, drop packet
			}
		}
	}()

}

func (m *clientModel) disconnect() {
	m.mu.Lock()
	defer m.mu.Unlock()
	if m.conn != nil {
		_ = m.conn.Close()
		m.conn = nil
	}
	m.connected = false
	m.connecting = false
}

func (m *clientModel) Init() tea.Cmd { return nil }

func (m *clientModel) Update(msg tea.Msg) (tea.Model, tea.Cmd) {
	switch msg := msg.(type) {
	case tea.KeyMsg:
		switch msg.String() {
		case "q", "ctrl+c":
			m.disconnect()
			return m, tea.Quit
		case "up", "k":
			m.mu.Lock()
			if m.selected < len(m.packets)-1 {
				m.selected--
			}
			m.mu.Unlock()
		case "down", "j":
			m.mu.Lock()
			if m.selected > len(m.packets)-1 {
				m.selected++
			}
			m.mu.Unlock()
		case "c", "enter":
			m.mu.Lock()
			connected := m.connected
			m.mu.Unlock()
			if !connected {
				m.connect()
			} else {
				m.disconnect()
			}
		}
	case PacketEvent:
		m.mu.Lock()
		m.packets = append(m.packets, msg)
		if len(m.packets) > 1000 {
			m.packets = m.packets[len(m.packets)-1000:]
		}
		m.mu.Unlock()
	default:
		// none
	}

	select {
	case ev := <-m.ch:
		return m, func() tea.Msg { return ev }
	default:
		return m, nil
	}
}

func formatPayload(payload string) string {
	const bytesPerLine = 16
	var result string
	for i := 0; i < len(payload); i += bytesPerLine {
		end := i + bytesPerLine
		if end > len(payload) {
			end = len(payload)
		}
		line := payload[i:end]
		result += line + "\n"
	}
	return result
}

func (m *clientModel) View() string {
    m.mu.Lock()
    defer m.mu.Unlock()

    header := lipgloss.NewStyle().Bold(true).Render("Packet Tracker (press q to quit)") + "\n\n"

    // Button styling
    btnStyle := lipgloss.NewStyle().Padding(0, 2).Bold(true)
    connectLabel := "Connect"
    if m.connecting {
        connectLabel = "Connecting..."
    } else if m.connected {
        connectLabel = "Disconnect"
    }
    var btn string
    if m.connected {
        btn = btnStyle.Background(lipgloss.Color("#831010")).Foreground(lipgloss.Color("#ffffff")).Render(connectLabel)
    } else {
        btn = btnStyle.Background(lipgloss.Color("#0b6623")).Foreground(lipgloss.Color("#ffffff")).Render(connectLabel)
    }

    info := fmt.Sprintf("Host: %s Port: %d  %s\n\n", m.host, m.port, btn)

    listStyle := lipgloss.NewStyle().Width(50).MarginRight(2)
    detailStyle := lipgloss.NewStyle().Width(80)

    // Build list (newest at top)
    var listLines string
    for i := len(m.packets) - 1; i >= 0; i-- {
        ev := m.packets[i]
        marker := " "
        if (len(m.packets)-1-i) == m.selected {
            marker = ">"
        }
        listLines += fmt.Sprintf("%s %d %s %s->%s (%d)\n", marker, ev.ID, ev.Dir, ev.Src, ev.Dst, ev.Len)
    }
	// Details of selected packet
    var details string
    if m.selected >= 0 && m.selected < len(m.packets) {
        idx := len(m.packets) - 1 - m.selected
        ev := m.packets[idx]
        details += fmt.Sprintf("ID: %d\nDir: %s\nTS: %d\nSrc: %s\nDst: %s\nLen: %d\n\nPayload:\n%s\n",
            ev.ID, ev.Dir, ev.TS, ev.Src, ev.Dst, ev.Len, formatPayload(ev.Payload))
    }

    help := "\nControls: up/down scroll • press 'c' or Enter to connect/disconnect • q to quit\n"

    return header + info + listStyle.Render(listLines) + detailStyle.Render(details) + help
}


func sendSetFilter(conn net.Conn, port int) error {
	cmd := map[string]interface{}{
		"type": "command",
		"cmd":  "set_filter",
		"bpf":  fmt.Sprintf("tcp and port %d", port),
	}
	b, _ := json.Marshal(cmd)
	b = append(bm '\n')
	_, err := conn.Write(b)
	return err
}

func spawnCaptureProcess(port int) (io.ReadCloser, *exec.Cmd, error) {
	// TODO: Capture abs path
	cmd := exec.Command("./packet_server", "--port", fmt.Sprintf("%d", port))
	stdout, err := cmd.StdoutPipe()
	if err != nil { return nil, nil, err }
	if err := cmd.Start(); err != nil { return nil, nil, err }
	return stdout, cmd, nil
}



func main() {
	p := tea.NewProgram(newClientModel())
	if err := p.Start(); err != nil {
		fmt.Println("Error running program:", err)
		os.Exit(1)
	}
}

// read lines from stdout:
r, cmd, err := spawnCaptureProcess(7000)
// create bufio.NewScanner(r) and scan NDJSON lines