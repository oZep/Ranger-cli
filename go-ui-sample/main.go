package main

import (
	"bufio"
	"encoding/json"
	"fmt"
	"net"
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
}

///
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
//



// base for bubbletea w lipgloss
func newClientModel() *clientModel {
	m := &clientModel{ch: make(chan PacketEvent, 64)}
	go m.runNetwork()
	return m
}

func (m *clientModel) Init() tea.Cmd { return nil }


func (m *clientModel) runNetwork() {
	for {
		conn, err := net.Dial("tcp", "127.0.0.1:9001")
		if err != nil {
			time.Sleep(1 * time.Second)
			continue
		}
		m.conn = conn
		reader := bufio.NewReader(conn)
		for {
			line, err := reader.ReadString('\n')
			if err != nil {
				conn.Close()
				break
			}
			var ev PacketEvent
			if err := json.Unmarshal([]byte(line), &ev); err != nil {
				// ignore parse errors for now; TODO: surface parse errors to UI
				continue
			}
			// send to UI via channel
			m.ch <- ev
		}
	}
}

// Update implements tea.Model for the client UI.
func (m *clientModel) Update(msg tea.Msg) (tea.Model, tea.Cmd) {
	switch msg := msg.(type) {
	case tea.KeyMsg:
		switch msg.String() {
		case "q", "ctrl+c":
			return m, tea.Quit
		case "up":
			if m.selected > 0 {
				m.selected--
			}
		case "down":
			m.mu.Lock()
			if m.selected < len(m.packets)-1 {
				m.selected++
			}
			m.mu.Unlock()
		}
	case PacketEvent:
		m.mu.Lock()
		m.packets = append(m.packets, msg)
		if len(m.packets) > 1000 {
			m.packets = m.packets[len(m.packets)-1000:]
		}
		m.mu.Unlock()
	default:
		// non-key messages from Bubble Tea (ignore)
	}
	// drain network channel into the model
	select {
	case ev := <-m.ch:
		// push as an internal message so Update can handle it
		return m, func() tea.Msg { return ev }
	default:
	}
	return m, nil
}

// View renders a very simple two-column UI: a list and details.
func (m *clientModel) View() string {
	m.mu.Lock()
	defer m.mu.Unlock()

	header := lipgloss.NewStyle().Bold(true).Render("Packet Tracker (press q to quit)") + "\n\n"
	listStyle := lipgloss.NewStyle().Width(50).MarginRight(2)
	detailStyle := lipgloss.NewStyle().Width(80)

	// Build list
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

	return header + listStyle.Render(listLines) + detailStyle.Render(details)
}

func main() {
	p := tea.NewProgram(newClientModel())
	if err := p.Start(); err != nil {
		fmt.Println("Error running program:", err)
		os.Exit(1)
	}
}
