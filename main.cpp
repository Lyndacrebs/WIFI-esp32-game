#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

static const char* AP_SSID = "ESP32-Game8";
static const char* AP_PASS = "12345678";

IPAddress local_IP(192,168,4,1);
IPAddress gateway(192,168,4,1);
IPAddress subnet(255,255,255,0);

WebServer server(80);

char board[9];
char turnPlayer = 'X';
bool gameOver = false;
char winner = ' ';
String playerX = "";
String playerO = "";

static const char INDEX_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="pt-br">
  <head>
    <meta charset="utf-8" />
    <meta
      name="viewport"
      content="width=device-width, initial-scale=1, viewport-fit=cover"
    />
    <title>Tic-Tac-Toe | ESP32</title>
    <style>
      :root {
        --bg: #0b0f19;
        --bg-grad1: #0b0f19;
        --bg-grad2: #111827;
        --card: rgba(255, 255, 255, 0.06);
        --glass: rgba(255, 255, 255, 0.08);
        --border: rgba(255, 255, 255, 0.18);
        --fg: #e6e6e6;
        --muted: #a8b3cf;
        --accent: #6ee7ff;
        --accent-2: #a78bfa;
        --ok: #34d399;
        --warn: #f59e0b;
        --err: #ef4444;
        --shadow: 0 10px 30px rgba(0, 0, 0, 0.35);
        --radius: 16px;
      }
      @media (prefers-color-scheme: light) {
        :root {
          --bg: #f7f8fb;
          --bg-grad1: #eef1f6;
          --bg-grad2: #f7f8fb;
          --card: rgba(255, 255, 255, 0.9);
          --glass: rgba(255, 255, 255, 0.8);
          --border: rgba(0, 0, 0, 0.08);
          --fg: #0e141b;
          --muted: #51607a;
          --accent: #0ea5e9;
          --accent-2: #7c3aed;
          --shadow: 0 8px 24px rgba(0, 0, 0, 0.08);
        }
      }
      html,
      body {
        margin: 0;
        color: var(--fg);
        background: radial-gradient(
            1200px 800px at 10% -10%,
            var(--bg-grad2) 0%,
            transparent 55%
          ),
          radial-gradient(
            1200px 800px at 110% 10%,
            var(--bg-grad2) 0%,
            transparent 55%
          ),
          linear-gradient(180deg, var(--bg-grad1), var(--bg));
        font-family: ui-sans-serif, system-ui, -apple-system, "Segoe UI", Roboto,
          Arial, "Noto Sans", "Liberation Sans", sans-serif;
        -webkit-font-smoothing: antialiased;
        text-rendering: optimizeLegibility;
        min-height: 100svh;
        padding-bottom: env(safe-area-inset-bottom);
      }
      .wrap {
        max-width: 680px;
        margin: clamp(16px, 4vw, 28px) auto;
        padding: clamp(12px, 2.5vw, 24px);
      }
      .card {
        background: var(--card);
        border: 1px solid var(--border);
        box-shadow: var(--shadow);
        backdrop-filter: blur(10px);
        border-radius: calc(var(--radius) + 4px);
        padding: clamp(14px, 2vw, 22px);
      }
      h1 {
        margin: 0 0 10px 0;
        font-size: clamp(20px, 2.2vw + 12px, 28px);
        letter-spacing: 0.3px;
        display: flex;
        align-items: baseline;
        gap: 10px;
      }
      .accent {
        color: var(--accent);
      }
      .info {
        display: flex;
        flex-wrap: wrap;
        gap: 10px;
        margin-bottom: 14px;
      }
      .tag {
        display: inline-flex;
        align-items: center;
        gap: 8px;
        padding: 6px 12px;
        font-size: 13px;
        border: 1px solid var(--border);
        border-radius: 999px;
        background: linear-gradient(180deg, var(--glass), transparent);
      }
      .you {
        border-color: color-mix(in oklab, var(--accent), white 20%);
        color: var(--accent);
      }
      .ok {
        color: var(--ok);
      }
      .warn {
        color: var(--warn);
      }
      .board {
        display: grid;
        grid-template-columns: repeat(3, 1fr);
        gap: clamp(8px, 1.8vw, 14px);
        margin: 10px 0 14px 0;
      }
      .cell {
        aspect-ratio: 1/1;
        display: flex;
        align-items: center;
        justify-content: center;
        font-weight: 700;
        font-stretch: 110%;
        font-size: clamp(40px, 6.8vw, 56px);
        letter-spacing: 0.5px;
        color: var(--fg);
        background: linear-gradient(
          180deg,
          rgba(255, 255, 255, 0.05),
          rgba(0, 0, 0, 0.08)
        );
        border: 1px solid var(--border);
        border-radius: calc(var(--radius) - 4px);
        box-shadow: var(--shadow);
        user-select: none;
        cursor: pointer;
        transition: transform 0.06s ease, box-shadow 0.2s ease,
          border-color 0.2s ease, background 0.2s ease, filter 0.2s ease;
        will-change: transform;
      }
      .cell:hover {
        transform: translateY(-1px) scale(1.01);
      }
      .cell:active {
        transform: scale(0.985);
      }
      .cell.disabled {
        opacity: 0.55;
        cursor: not-allowed;
        filter: saturate(0.6);
        transform: none !important;
      }
      .board:has(.cell:not(.disabled)):not(.game-over) .cell:not(.disabled) {
        animation: pulse 1.6s ease-in-out infinite;
      }
      @keyframes pulse {
        0%,
        100% {
          box-shadow: var(--shadow);
        }
        50% {
          box-shadow: 0 0 0 6px
              color-mix(in oklab, var(--accent), transparent 82%),
            var(--shadow);
        }
      }
      .row {
        display: flex;
        flex-wrap: wrap;
        align-items: center;
        gap: 10px;
        margin: 6px 0 2px;
      }
      button {
        appearance: none;
        border: 1px solid var(--border);
        background: linear-gradient(180deg, var(--glass), transparent);
        color: var(--fg);
        padding: 10px 14px;
        border-radius: 12px;
        font-size: 14px;
        letter-spacing: 0.2px;
        cursor: pointer;
        box-shadow: var(--shadow);
        transition: transform 0.06s ease, border-color 0.2s ease,
          background 0.2s ease, box-shadow 0.2s ease;
      }
      button:hover {
        transform: translateY(-1px);
        border-color: #6b7280;
      }
      button:active {
        transform: translateY(0);
      }
      .muted {
        color: var(--muted);
        font-size: 14px;
      }
      .hr {
        height: 1px;
        background: var(--border);
        margin: 14px 0;
        border-radius: 1px;
      }
      @media (prefers-reduced-motion: reduce) {
        * {
          animation: none !important;
          transition: none !important;
        }
      }
      .cell:focus-visible,
      button:focus-visible {
        outline: 2px solid color-mix(in oklab, var(--accent), white 10%);
        outline-offset: 3px;
      }
      @media (max-width: 380px) {
        .info .tag {
          font-size: 12px;
          padding: 5px 10px;
        }
        button {
          padding: 9px 12px;
        }
      }
    </style>
  </head>
  <body>
    <div class="wrap">
      <div class="card">
        <h1>
          Jogo da Velha • <span id="role" class="accent">Conectando…</span>
        </h1>
        <div class="info">
          <span class="tag" id="players">Jogadores: 0/2</span>
          <span class="tag" id="turn">Vez: —</span>
          <span class="tag you" id="you">Você: —</span>
        </div>
        <div class="board" id="grid"></div>
        <div class="row">
          <button id="resetBtn" title="Reiniciar a partida">Reiniciar</button>
          <span class="muted" id="status">Aguardando…</span>
        </div>
        <div class="hr"></div>
        <p class="muted">
          Abra este endereço em dois celulares conectados ao Wi-Fi “ESP32-Game”.
        </p>
      </div>
    </div>
    <script>
      const grid = document.getElementById("grid");
      const roleEl = document.getElementById("role");
      const playersEl = document.getElementById("players");
      const turnEl = document.getElementById("turn");
      const youEl = document.getElementById("you");
      const statusEl = document.getElementById("status");
      const resetBtn = document.getElementById("resetBtn");
      let myRole = null;
      const cells = [];
      for (let i = 0; i < 9; i++) {
        const d = document.createElement("div");
        d.className = "cell";
        d.dataset.idx = i;
        d.tabIndex = 0;
        d.onclick = () => tryMove(i);
        d.onkeydown = (e) => {
          if (e.key === "Enter" || e.key === " ") {
            e.preventDefault();
            tryMove(i);
          }
        };
        grid.appendChild(d);
        cells.push(d);
      }
      async function join() {
        try {
          const r = await fetch("/join");
          const j = await r.json();
          myRole = j.role || null;
          roleEl.textContent = myRole ? `Você é ${myRole}` : "Espectador";
          youEl.textContent = myRole ? `Você: ${myRole}` : "Você: —";
        } catch (e) {
          roleEl.textContent = "Falha ao conectar";
        }
      }
      async function getState() {
        const r = await fetch("/state", { cache: "no-store" });
        return r.json();
      }
      async function tryMove(i) {
        if (myRole == null) return;
        const st = await getState();
        if (st.gameOver) return;
        if (st.turn !== myRole) return;
        if (st.board[i] !== " ") return;
        const r = await fetch(`/move?cell=${i}&role=${myRole}`, {
          cache: "no-store",
        });
        const j = await r.json();
        render(j);
      }
      function render(st) {
        st.board.forEach((c, i) => {
          cells[i].textContent = c === " " ? "" : c;
          const can = !st.gameOver && st.turn === myRole && c === " ";
          cells[i].classList.toggle("disabled", !can);
        });
        grid.classList.toggle("game-over", !!st.gameOver);
        playersEl.textContent = `Jogadores: ${st.players}/2`;
        turnEl.textContent = `Vez: ${st.gameOver ? "—" : st.turn}`;
        if (st.gameOver) {
          statusEl.textContent =
            st.winner === "D" ? "Empate!" : `Vitória de ${st.winner}`;
          statusEl.classList.toggle("ok", st.winner !== "D");
        } else {
          statusEl.textContent = st.turn === myRole ? "Sua vez!" : "Aguarde…";
          statusEl.classList.remove("ok");
        }
      }
      async function poll() {
        try {
          const st = await getState();
          render(st);
        } catch (e) {}
      }
      resetBtn.onclick = async () => {
        await fetch("/reset", { cache: "no-store" });
        await poll();
      };
      (async () => {
        await join();
        await poll();
        setInterval(poll, 600);
      })();
    </script>
  </body>
</html>

)HTML";

static void resetGame(){
  for(int i=0;i<9;i++) board[i]=' ';
  turnPlayer='X';
  gameOver=false;
  winner=' ';
}

static int checkWinner(){
  const int wins[8][3]={{0,1,2},{3,4,5},{6,7,8},{0,3,6},{1,4,7},{2,5,8},{0,4,8},{2,4,6}};
  for(const auto&w:wins){char a=board[w[0]],b=board[w[1]],c=board[w[2]];if(a!=' '&&a==b&&b==c) return a;}
  bool full=true;for(int i=0;i<9;i++) if(board[i]==' '){full=false;break;}
  return full?'D':' ';
}

static String jsonState(){
  int countPlayers=(playerX!=""?1:0)+(playerO!=""?1:0);
  String j="{";j+="\"board\":[";
  for(int i=0;i<9;i++){j+="\"";j+=(board[i]==' '? ' ': board[i]);j+="\"";if(i<8) j+=",";}
  j+="],";j+="\"turn\":\"";j+=turnPlayer;j+="\",";j+="\"gameOver\":";j+=(gameOver?"true":"false");j+=",";
  j+="\"winner\":\"";j+=(winner==' ' ? ' ' : winner);j+="\",";j+="\"players\":";j+=countPlayers;j+="}";
  return j;
}

static String clientID(){
  IPAddress ip=server.client().remoteIP();
  return String(ip[0])+"."+String(ip[1])+"."+String(ip[2])+"."+String(ip[3]);
}

static void handleRoot(){server.send_P(200,"text/html; charset=utf-8",INDEX_HTML);}
static void handleJoin(){
  String me=clientID();
  if(playerX==me||playerO==me){char role=(playerX==me)?'X':'O';server.send(200,"application/json",String("{\"role\":\"")+role+"\"}");return;}
  if(playerX==""){playerX=me;server.send(200,"application/json","{\"role\":\"X\"}");return;}
  if(playerO==""){playerO=me;server.send(200,"application/json","{\"role\":\"O\"}");return;}
  server.send(200,"application/json","{\"role\":null}");
}
static void handleState(){server.send(200,"application/json",jsonState());}
static void handleMove(){
  if(!server.hasArg("cell")||!server.hasArg("role")){server.send(400,"application/json","{\"err\":\"params\"}");return;}
  int cell=server.arg("cell").toInt();char role=server.arg("role")[0];String me=clientID();
  if(gameOver||cell<0||cell>8||(role!='X'&&role!='O')){server.send(200,"application/json",jsonState());return;}
  if((role=='X'&&playerX!=me)||(role=='O'&&playerO!=me)){server.send(200,"application/json",jsonState());return;}
  if(turnPlayer!=role||board[cell]!=' '){server.send(200,"application/json",jsonState());return;}
  board[cell]=role;int w=checkWinner();
  if(w=='X'||w=='O'){winner=(char)w;gameOver=true;}
  else if(w=='D'){winner='D';gameOver=true;}
  else{turnPlayer=(turnPlayer=='X')?'O':'X';}
  server.send(200,"application/json",jsonState());
}
static void handleReset(){resetGame();server.send(200,"application/json",jsonState());}
static void handleNotFound(){server.send(404,"text/plain","Not found");}

void setup(){
  resetGame();
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(local_IP,gateway,subnet);
  WiFi.softAP(AP_SSID,AP_PASS);
  server.on("/",handleRoot);
  server.on("/join",handleJoin);
  server.on("/state",handleState);
  server.on("/move",handleMove);
  server.on("/reset",handleReset);
  server.onNotFound(handleNotFound);
  server.begin();
}

void loop(){server.handleClient();}
