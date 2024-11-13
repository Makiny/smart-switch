/**
 * @author kaiyin
 */

/**
 * 获取电量统计数据
 * @returns {Promise<any>}
 */
export const fetchEnergyRowData = async () => {
    try {
        const response = await fetch('/api/energy/statistics/get', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
        });
        return await response.json();
    } catch (error) {
        console.error('获取 rowData 失败:', error);
        throw error;
    }
};


/**
 * 获取用电状态
 * @returns {Promise<any|null>}
 * @param body
 */
export async function getDeviceStatus(body) {
    try {
        const response = await fetch('/api/status', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify(body)
        });
        return await response.json();
    } catch (error) {
        console.error('Error get energy status:', error);
        return null;
    }
}

export async function getDeviceStatusOnce(onUpdate, body) {
    const data = await getDeviceStatus(body);
    if (data) {
        onUpdate(data);
    }
}

let intervalIdGetDeviceStatus = null;

/**
 * 定期获取实时数据
 * @param updateInterval
 * @param query
 * @param onUpdate
 * @returns {Promise<void>}
 */
export async function startGetDeviceStatus(updateInterval, onUpdate, body) {
    if (intervalIdGetDeviceStatus !== null) {
        clearInterval(intervalIdGetDeviceStatus);
    }

    intervalIdGetDeviceStatus = setInterval(async () => {
        const data = await getDeviceStatus(body);
        if (data) {
            onUpdate(data);
        }
    }, updateInterval);
}

/**
 * 停止实时数据更新
 */
export function stopGetDeviceStatus() {
    if (intervalIdGetDeviceStatus !== null) {
        clearInterval(intervalIdGetDeviceStatus);
        intervalIdGetDeviceStatus = null;
    }
}

/**
 * 获取WiFi列表
 * @returns {Promise<any>}
 */
export async function getWifiList() {
    try {
        const response = await fetch(`/api/wifi?action=list`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            }
        });
        return await response.json();
    } catch (error) {
        console.error('发送凭据失败:', error);
        throw error;
    }
}

/**
 * 连接WiFi
 * @param ssid
 * @param cred
 * @returns {Promise<any>}
 */
export async function connectWifiByCred(ssid, cred) {
    try {
        const response = await fetch(`/api/wifi?action=connect`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                'ssid': ssid,
                'cred': cred
            })
        });
        return await response.json();
    } catch (error) {
        console.error('发送凭据失败:', error);
        throw error;
    }
}

export async function getWifiConnectStatus() {

}

/**
 *  获取设备配置
 * @returns {Promise<any>}
 */
export async function getConfig() {
    try {
        const response = await fetch(`/api/config?action=get`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
        });
        return await response.json();
    } catch (error) {
        console.error('获取配置失败:', error);
        throw error;
    }
}

/**
 * 更新设备配置
 * @param config
 * @returns {Promise<any>}
 */
export async function updateConfig(config) {
    try {
        const response = await fetch(`/api/config?action=update`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify(config)
        });
        return await response.json();
    } catch (error) {
        console.error('更新配置失败:', error);
        throw error;
    }
}

/**
 * 重置设备
 * @param reset
 * @returns {Promise<any>}
 */
export async function resetDevice(reset) {
    try {
        const response = await fetch(`/api/device?action=reset`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                'rstMode': reset,
            })
        });
        return await response.json();
    } catch (error) {
        console.error('重置失败:', error);
        throw error;
    }
}

/**
 * 获取设备信息
 * @returns {Promise<any>}
 */
export async function getDeviceInfo() {
    try {
        const response = await fetch(`/api/device?action=info`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
        });
        return await response.json();
    } catch (error) {
        console.error('获取设备信息失败:', error);
        throw error;
    }
}

/**
 * 设备控制
 * @returns {Promise<any>}
 */
export async function deviceControl(ctrlCmd) {
    try {
        const response = await fetch(`/api/device?action=ctrl`, {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({
                'ctrlCmd': ctrlCmd,
            })
        });
        return await response.json();
    } catch (error) {
        console.error('获取设备信息失败:', error);
        throw error;
    }
}