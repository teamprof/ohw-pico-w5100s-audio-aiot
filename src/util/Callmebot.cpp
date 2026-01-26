/* Copyright 2026 teamprof.net@gmail.com
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <UrlEncode.h>
#include "Callmebot.h"
#include "../ArduProfApp.h"

////////////////////////////////////////////////////////////////////////////////////////////
const char Callmebot::_apiHost[] = CALLMEBOT_HOST;
const char Callmebot::_apiPath[] = CALLMEBOT_PATH;
const int Callmebot::_apiPort = CALLMEBOT_PORT;

///////////////////////////////////////////////////////////////////////////////
Callmebot::Callmebot(StateCallback callback) : _tcpClient(),
                                               _tcpState(Ready),
                                               _isHttpStatusLineReceived(false),
                                               _connectTimeout(0),
                                               _callback(callback)
{
}

void Callmebot::send(const char *text)
{
    LOG_TRACE("sending:", text);
    prepareText(text);

    uint8_t sn_ir = SnIR::SEND_OK | SnIR::TIMEOUT | SnIR::RECV | SnIR::DISCON | SnIR::CON;
    if (_tcpClient.connect(_apiHost, _apiPort, sn_ir, onTcpClientEvent))
    {
        LOG_TRACE("connecting to server=", _apiHost, ", port=", _apiPort);
        _tcpState = Connecting;
        _connectTimeout = 0;
        _isHttpStatusLineReceived = false;
        invokeCallbackIfNotNull(Sending);
    }
    else
    {
        LOG_TRACE("Fail to connect server=", _apiHost, ", port=", _apiPort);
        invokeCallbackIfNotNull(SentFail);
    }
}

void Callmebot::setStateCallback(StateCallback callback)
{
    _callback = callback;
}

void Callmebot::update(void)
{
    auto state = _tcpState;
    switch (state)
    {
    case Ready:
        break;

    case Connecting:
    {
        if (_tcpClient.connected())
        {
            LOG_TRACE("Connected server: IP=", _tcpClient.remoteIP(), ", port=", _tcpClient.remotePort());
            writeText(_messageText);
            _tcpState = Connected;
            _connectTimeout = 0;
        }
        else if (_connectTimeout++ >= CONNECT_TIMEOUT)
        {
            LOG_TRACE("Connecting: timeout! _connectTimeout=", _connectTimeout, " seconds");
            _tcpState = Ready;
            _tcpClient.stop();
            invokeCallbackIfNotNull(SentFail);
        }
        else
        {
            LOG_TRACE("Connecting: time=", _connectTimeout, " seconds");
        }
        break;
    }

    case Connected:
    {
        int responseCode;
        if (readHttpResponse(&responseCode))
        {
            if (responseCode == 200)
            {
                invokeCallbackIfNotNull(SentSuccess);
            }
            else
            {
                LOG_TRACE("Connected: HTTP server response statusCode=", responseCode);
                invokeCallbackIfNotNull(SentFail);
            }
            _tcpState = Ready;
            _tcpClient.stop();
        }
        else if (_connectTimeout++ >= CONNECT_TIMEOUT)
        {
            LOG_TRACE("Connected: timeout! _connectTimeout=", _connectTimeout, " seconds");
            invokeCallbackIfNotNull(SentFail);
            _tcpState = Ready;
            _tcpClient.stop();
        }
        else
        {
            LOG_TRACE("Connected: time=", _connectTimeout, " seconds");
        }
        break;
    }

    default:
        LOG_TRACE("unsupported TCP state: ", (int)state);
        break;
    }
}

void Callmebot::onTcpClientEvent(uint8_t sr_ir)
{
    if (sr_ir & SnIR::SEND_OK)
    {
        LOG_DEBUG("SnIR::SEND_OK");
    }
    if (sr_ir & SnIR::TIMEOUT)
    {
        LOG_DEBUG("SnIR::TIMEOUT");
    }
    if (sr_ir & SnIR::RECV)
    {
        LOG_DEBUG("SnIR::RECV");
    }
    if (sr_ir & SnIR::DISCON)
    {
        LOG_DEBUG("SnIR::DISCON");
    }
    if (sr_ir & SnIR::CON)
    {
        LOG_DEBUG("SnIR::CON");
    }
}

bool Callmebot::readHttpResponse(int *ptrResponseCode)
{
    int tcpSize = _tcpClient.available();
    while (tcpSize > 0)
    {
        LOG_DEBUG("tcpSize=", tcpSize);
        if (tcpSize >= sizeof(_shareRxBuf))
        {
            tcpSize = sizeof(_shareRxBuf) - 1; // truncate data if oversize
        }
        _tcpClient.read(_shareRxBuf, tcpSize);
        _shareRxBuf[tcpSize] = '\0';
        LOG_DEBUG("Received ", tcpSize, " bytes from ", _tcpClient.remoteIP(), ":", _tcpClient.remotePort());
        LOG_DEBUG("Content: ", (const char *)_shareRxBuf);

        if (!_isHttpStatusLineReceived)
        {
            char *strResponseCode = strstr((char *)_shareRxBuf, "HTTP/");
            if (strResponseCode)
            {
                _isHttpStatusLineReceived = true;

                int responseCode;
                sscanf(strResponseCode, "HTTP/%*d.%*d %d", &responseCode);
                LOG_DEBUG("HTTP Status Code: ", responseCode);

                *ptrResponseCode = responseCode;
                return true;
            }
        }

        tcpSize = _tcpClient.available();
    }
    LOG_DEBUG("tcpSize=", tcpSize);
    return false;
}

void Callmebot::writeText(const char *text)
{
    // LOG_DEBUG("_messageText=", text);

    // Make a HTTP request:
    _tcpClient.print("GET ");
    _tcpClient.print(text);
    _tcpClient.println(" HTTP/1.1");
    _tcpClient.print("Host: ");
    _tcpClient.println(_apiHost);
    _tcpClient.println("Connection: close");
    _tcpClient.println();
}

// make text data to be sent
void Callmebot::prepareText(const char *text)
{
    String encoded = urlEncode(text);
    snprintf(_messageText, sizeof(_messageText), "%s%s", _apiPath, encoded.c_str());
    // LOG_TRACE("_messageText=", _messageText);
}
