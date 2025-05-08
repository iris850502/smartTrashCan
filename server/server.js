const express = require('express');
const fs = require('fs');
const path = require('path');
const app = express();
const PORT = 3000;
const STATUS_PATH = `${process.env.HOME}/smartTrashCan/status`;
const CO_FILE = `${process.env.HOME}/smartTrashCan/status/co_status.txt`;

app.use(express.json());
app.use(express.static('public'));
app.use('/status', express.static(path.join(STATUS_PATH)));


// API: 讀CO濃度狀態
app.get('/api/co', (req, res) => {
    fs.readFile(`${STATUS_PATH}/co_status.txt`, 'utf8', (err, data) => {
        if (err) {
            return res.status(500).send('Error reading CO file');
        }

		const lines = data.trim().split('\n');
        let status = lines[0];
        let value = lines[1];
		const updated_at = parseInt(lines[2]); //C寫進檔案的時間
		const danger = (lines[3] && lines[3].trim() === "DANGER") ? true : false;


        // 再拿檔案更新時間
        fs.stat(CO_FILE, (err, stats) => {
            if (err) {
                return res.status(500).send('Error reading CO file stats');
            }

            const mtime = Math.floor(stats.mtimeMs / 1000); // 檔案最後修改時間（秒）
            const now = Math.floor(Date.now() / 1000);     // 現在時間（秒）
            const age = now - mtime; // 差幾秒

            if (age > 10) { // 超過10秒沒更新，就自動回復NORMAL
                status = "NORMAL";
                value = 0;
            }

            res.json({
                status: status,
                value: value,
                updated_at: updated_at,
				danger: danger
            });
        });
    });
});

// API: 讀垃圾桶TCRT狀態
app.get('/api/tcrt', (req, res) => {
    const filePath = `${STATUS_PATH}/tcrt_status.txt`;

    fs.readFile(filePath, 'utf8', (err, data) => {
        if (err) {
            return res.status(500).send('Error reading TCRT file');
        }

        const lines = data.trim().split('\n');
        const status = lines[0];
        const updated_at = parseInt(lines[1]); // 來自 C 程式寫入的時間

        const now = Math.floor(Date.now() / 1000);
        const age = now - updated_at;

        if (age > 15) {
            // 超過 15 秒沒更新（程式可能中斷）
            return res.json({ status: "INACTIVE" });
        }

        res.json({
            status: status
        });
    });
});

// API: 感測器POST新垃圾桶狀態
app.post('/api/tcrt/update', (req, res) => {
    if (req.body.sensor !== undefined) {
        const status = req.body.sensor === 0 ? 'FULL' : 'NORMAL';
        console.log('Received TCRT update:', status);

        // 寫進tcrt_alert.txt
        fs.writeFile(`${STATUS_PATH}/tcrt_status.txt`, status, (err) => {
            if (err) {
                console.error('Failed to write TCRT status file');
                return res.status(500).send('Error writing TCRT file');
            }
            res.send('OK');
        });
    } else {
        res.status(400).send('Bad Request: sensor field missing');
    }
});

app.listen(PORT, () => {
    console.log(`Server running on http://localhost:${PORT}`);
});

// API: 感測器POST蓋子狀態
app.post('/api/lid/update', (req, res) => {
    const status = req.body.status;
    if (!status) {
        return res.status(400).send('Missing lid status');
    }

    const data = `${status}\n${Math.floor(Date.now() / 1000)}`;
    const lidFilePath = `${STATUS_PATH}/lid_status.txt`;

    fs.writeFile(lidFilePath, data, (err) => {
        if (err) {
            console.error('Failed to write lid status file');
            return res.status(500).send('Error writing lid status');
        }
        console.log('Received lid update:', status);
        res.send('OK');
    });
});

