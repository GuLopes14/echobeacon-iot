// ===== Dependências =====
const express = require("express");
const bodyParser = require("body-parser");
const cors = require("cors");
const sqlite3 = require("sqlite3").verbose();
const mqtt = require("mqtt");

const app = express();
app.use(bodyParser.json());
app.use(cors()); // ✅ IMPORTANTE: Permite requisições do Node-RED

// ===== Banco de Dados =====
const db = new sqlite3.Database("./echobeacon.db");

// Tabelas principais
db.serialize(() => {
  db.run(`CREATE TABLE IF NOT EXISTS motos (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    placa TEXT UNIQUE,
    modelo TEXT,
    chassi TEXT,
    problema TEXT
  )`);

  db.run(`CREATE TABLE IF NOT EXISTS ativacoes (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    moto_id INTEGER,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    status TEXT,
    FOREIGN KEY(moto_id) REFERENCES motos(id)
  )`);

  // ✅ Insere motos de exemplo se não existirem
  const motos = [
    { placa: "AAA1111", modelo: "MOTTU_E1", chassi: "CHASSI111", problema: null },
    { placa: "BBB2222", modelo: "MOTTU_E2", chassi: "CHASSI222", problema: null },
    { placa: "CCC3333", modelo: "MOTTU_E3", chassi: "CHASSI333", problema: null }
  ];

  motos.forEach(moto => {
    db.run(
      "INSERT OR IGNORE INTO motos (placa, modelo, chassi, problema) VALUES (?,?,?,?)",
      [moto.placa, moto.modelo, moto.chassi, moto.problema]
    );
  });
});

// ===== MQTT =====
const client = mqtt.connect("mqtt://broker.hivemq.com:1883");

client.on("connect", () => {
  console.log("📡 Conectado ao broker MQTT");
  // ✅ Escuta comandos para registrar ativações
  client.subscribe("fiap/iot/echobeacon/comando");
  console.log("📬 Inscrito em: fiap/iot/echobeacon/comando");
});

client.on("message", (topic, message) => {
  try {
    const payload = JSON.parse(message.toString());
    console.log("📥 Comando recebido:", payload);

    if (payload.comando === "ativar" && payload.moto) {
      const { placa, modelo, chassi } = payload.moto;
      
      // ✅ Localiza moto pela placa
      db.get("SELECT id FROM motos WHERE placa = ?", [placa], (err, row) => {
        if (row) {
          // Moto existe - registra ativação
          db.run(
            "INSERT INTO ativacoes (moto_id, status) VALUES (?, ?)",
            [row.id, "ativado"],
            function(err) {
              if (err) {
                console.error("❌ Erro ao registrar ativação:", err.message);
              } else {
                console.log(`✅ Ativação registrada: ${placa} - ID: ${this.lastID}`);
              }
            }
          );
        } else {
          // ✅ Moto não existe - cadastra automaticamente
          console.log(`🆕 Cadastrando nova moto: ${placa}`);
          db.run(
            "INSERT INTO motos (placa, modelo, chassi, problema) VALUES (?,?,?,?)",
            [placa, modelo, chassi, null],
            function(err) {
              if (err) {
                console.error("❌ Erro ao cadastrar moto:", err.message);
              } else {
                console.log(`✅ Moto cadastrada: ${placa} - ID: ${this.lastID}`);
                
                // Agora registra a ativação
                db.run(
                  "INSERT INTO ativacoes (moto_id, status) VALUES (?, ?)",
                  [this.lastID, "ativado"],
                  function(err) {
                    if (err) {
                      console.error("❌ Erro ao registrar ativação:", err.message);
                    } else {
                      console.log(`✅ Ativação registrada: ${placa} - ID: ${this.lastID}`);
                    }
                  }
                );
              }
            }
          );
        }
      });
    }
  } catch (e) {
    console.error("❌ Erro ao processar mensagem MQTT:", e.message);
  }
});

// ===== API REST =====

// Registrar nova moto
app.post("/motos", (req, res) => {
  const { placa, modelo, chassi, problema } = req.body;
  
  if (!placa || !modelo || !chassi) {
    return res.status(400).json({ error: "Placa, modelo e chassi são obrigatórios" });
  }

  db.run(
    "INSERT INTO motos (placa, modelo, chassi, problema) VALUES (?,?,?,?)",
    [placa, modelo, chassi, problema || null],
    function (err) {
      if (err) {
        return res.status(400).json({ error: err.message });
      }
      res.json({ 
        success: true,
        id: this.lastID,
        placa: placa 
      });
    }
  );
});

// Listar todas as motos
app.get("/motos", (req, res) => {
  db.all("SELECT * FROM motos ORDER BY placa", [], (err, rows) => {
    if (err) {
      return res.status(500).json({ error: err.message });
    }
    res.json(rows);
  });
});

// Histórico de ativações
app.get("/ativacoes", (req, res) => {
  db.all(
    `SELECT 
       a.id, 
       m.placa, 
       m.modelo,
       m.chassi,
       a.timestamp, 
       a.status
     FROM ativacoes a
     JOIN motos m ON a.moto_id = m.id
     ORDER BY a.timestamp DESC
     LIMIT 100`,
    [],
    (err, rows) => {
      if (err) {
        return res.status(500).json({ error: err.message });
      }
      res.json(rows);
    }
  );
});

// Ativações de uma moto específica
app.get("/ativacoes/:placa", (req, res) => {
  const { placa } = req.params;
  
  db.all(
    `SELECT 
       a.id, 
       m.placa, 
       m.modelo,
       a.timestamp, 
       a.status
     FROM ativacoes a
     JOIN motos m ON a.moto_id = m.id
     WHERE m.placa = ?
     ORDER BY a.timestamp DESC`,
    [placa],
    (err, rows) => {
      if (err) {
        return res.status(500).json({ error: err.message });
      }
      res.json(rows);
    }
  );
});

// ✅ Endpoint de saúde
app.get("/health", (req, res) => {
  res.json({ 
    status: "ok", 
    mqtt: client.connected ? "connected" : "disconnected",
    timestamp: new Date().toISOString()
  });
});

// ===== Graceful Shutdown =====
process.on("SIGINT", () => {
  console.log("\n🛑 Encerrando servidor...");
  client.end();
  db.close();
  process.exit(0);
});

// ===== Start Server =====
const PORT = 3000;
app.listen(PORT, () => {
  console.log(`🚀 API rodando em http://localhost:${PORT}`);
  console.log(`📊 Ativações: http://localhost:${PORT}/ativacoes`);
  console.log(`🏍️  Motos: http://localhost:${PORT}/motos`);
});