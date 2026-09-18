// Trae / DeepSeek 通路诊断与修复工具
// 构建: csc.exe /target:winexe /codepage:65001 /out:TraeNetDoctor.exe TraeNetDoctor.cs
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Net;
using System.Net.NetworkInformation;
using System.Net.Security;
using System.Net.Sockets;
using System.Security.Principal;
using System.Security.Authentication;
using System.Text;
using System.Threading;
using System.Windows.Forms;
using System.Management;
using Microsoft.Win32;

namespace TraeNetDoctor
{
    static class Program
    {
        [STAThread]
        static void Main(string[] args)
        {
            try
            {
                // 管理员子进程模式：执行修复、写日志、退出（不创建窗口）
                if (args.Length > 0 && args[0] == "--repair")
                {
                    string tasks = args.Length > 1 ? args[1] : "";
                    string log = "";
                    for (int i = 0; i < args.Length - 1; i++)
                        if (args[i] == "--log") log = args[i + 1];
                    RepairRunner.Run(tasks, log);
                    return;
                }

                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                Application.Run(new MainForm());
            }
            catch (Exception ex)
            {
                try
                {
                    File.WriteAllText(Path.Combine(Path.GetTempPath(), "TraeNetDoctor_error.log"),
                        DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss") + "\r\n" + ex.ToString());
                }
                catch { }
                try { MessageBox.Show("程序启动失败：\r\n\r\n" + ex.ToString(), "错误", MessageBoxButtons.OK, MessageBoxIcon.Error); }
                catch { }
            }
        }
    }

    // =======================================================================
    //  一、基础探测能力
    // =======================================================================
    static class Net
    {
        public const int PingTimeout = 1500;

        // TLS 协议选择。（重要）
        // 坑点：用 csc 编译且不带 app.config 的 exe，运行时被当作"4.0 兼容模式"，
        //   SslStream 的默认协议会降级成 TLS 1.0，被现代 CDN 拒绝（调用 SSPI 失败）；
        //   而 SslProtocols.None 会被直接判为非法参数（ArgumentException）。
        // 实测结论（无 app.config 的 exe）：
        //   Default       -> TLS 1.0，trae.cn 握手失败
        //   None          -> ArgumentException，完全不可用
        //   Tls12         -> 可用，但只有 TLS 1.2
        //   Tls12 | Tls13 -> 可用且协商到 TLS 1.3   <= 用这个
        // 若某个更老的运行时不接受 Tls13 位，会自动降级为纯 TLS 1.2 并记住，
        // 因此不需要任何外部配置文件，单独一个 exe 就能正常工作。
        static SslProtocols UseProtocols = SslProtocols.Tls12 | SslProtocols.Tls13;
        static readonly object ProtoLock = new object();

        public static string ProtocolName()
        {
            SslProtocols p;
            lock (ProtoLock) p = UseProtocols;
            if (p == SslProtocols.Tls12) return "TLS 1.2（本运行时不支持 TLS 1.3，已自动降级）";
            return "TLS 1.2 + 1.3（显式指定）";
        }

        // 在已建立连接的 SslStream 上完成 TLS 握手；必要时自动降级协议档位后重试
        static void Handshake(SslStream ssl, string sniHost)
        {
            Exception last = null;
            for (int attempt = 0; attempt < 3; attempt++)
            {
                SslProtocols use;
                lock (ProtoLock) use = UseProtocols;
                try
                {
                    ssl.AuthenticateAsClient(sniHost, null, use, false);
                    return;
                }
                catch (ArgumentException ex)
                {
                    // 参数校验失败发生在发出任何数据之前，所以可以在同一个连接上重试
                    last = ex;
                    lock (ProtoLock)
                    {
                        if (UseProtocols != SslProtocols.Tls12) UseProtocols = SslProtocols.Tls12;
                        else throw;
                    }
                }
            }
            if (last != null) throw last;
        }

        public static PingReply Ping(string host, int timeout)
        {
            try
            {
                using (Ping p = new Ping()) return p.Send(host, timeout);
            }
            catch { return null; }
        }

        // 带 DF 标志的定长探测，用于二分查找路径 MTU
        public static bool PingSize(string host, int size, int timeout)
        {
            try
            {
                byte[] buf = new byte[size];
                PingOptions opt = new PingOptions(64, true);
                using (Ping p = new Ping())
                {
                    PingReply r = p.Send(host, timeout, buf, opt);
                    return r != null && r.Status == IPStatus.Success;
                }
            }
            catch { return false; }
        }

        // ICMP 载荷 1200~1472 二分，MTU = 载荷 + 28
        public static int PathMtu(string host, int timeout)
        {
            if (!PingSize(host, 1200, timeout)) return -1;
            int lo = 1200, hi = 1472, best = 1200;
            while (lo <= hi)
            {
                int mid = (lo + hi) / 2;
                if (PingSize(host, mid, timeout)) { best = mid; lo = mid + 1; }
                else hi = mid - 1;
            }
            return best + 28;
        }

        public static bool TcpPort(string host, int port, int timeoutMs)
        {
            TcpClient c = null;
            try
            {
                c = new TcpClient();
                IAsyncResult ar = c.BeginConnect(host, port, null, null);
                if (!ar.AsyncWaitHandle.WaitOne(timeoutMs, false)) return false;
                c.EndConnect(ar);
                return true;
            }
            catch { return false; }
            finally { if (c != null) try { c.Close(); } catch { } }
        }

        public class TlsResult
        {
            public bool Ok;
            public long Ms;
            public string Protocol = "", Error = "";
        }

        // 只做一次 TLS 握手（不写业务请求），用于连续多次判定握手稳定性
        public static TlsResult TlsHandshake(string host, int port, int timeoutMs)
        {
            return TlsHandshake(host, host, port, timeoutMs);
        }

        // connectHost 可以是某个具体的边缘 IP，sniHost 仍用域名（保证 SNI/证书校验对象正确）
        public static TlsResult TlsHandshake(string connectHost, string sniHost, int port, int timeoutMs)
        {
            TlsResult r = new TlsResult();
            TcpClient c = null;
            SslStream s = null;
            try
            {
                c = new TcpClient();
                IAsyncResult ar = c.BeginConnect(connectHost, port, null, null);
                if (!ar.AsyncWaitHandle.WaitOne(timeoutMs, false)) { r.Error = "TCP 连接超时"; return r; }
                c.EndConnect(ar);
                Stopwatch sw = Stopwatch.StartNew();
                s = new SslStream(c.GetStream(), false, delegate { return true; });
                s.ReadTimeout = timeoutMs;
                s.WriteTimeout = timeoutMs;
                Handshake(s, sniHost);
                r.Ms = sw.ElapsedMilliseconds;
                r.Protocol = s.SslProtocol.ToString();
                r.Ok = true;
            }
            catch (Exception ex)
            {
                r.Error = ex.Message;
                if (ex.InnerException != null) r.Error += " (" + ex.InnerException.Message + ")";
            }
            finally
            {
                if (s != null) try { s.Dispose(); } catch { }
                if (c != null) try { c.Close(); } catch { }
            }
            return r;
        }

        public class HttpResult
        {
            public bool Ok;
            public long TcpMs = -1, TlsMs = -1, TtfbMs = -1, ReadMs = -1;
            public int Bytes, Stalls;
            public string Protocol = "", Issuer = "", FirstLine = "", Snippet = "", Error = "";
        }

        // 完整跑一次 HTTPS 请求：TCP -> TLS -> 写请求 -> 读响应
        // 关键指标：TtfbMs(首字节耗时) / Bytes(实收字节) / Stalls(读超时次数)
        // 若 ping 通但 Bytes=0 且 Stalls>0，即为数据面黑洞（大包被丢或 TLS 被劫持）
        public static HttpResult Https(string server, int port, string method, string path,
            Dictionary<string, string> headers, string body,
            int connectTimeoutMs, int readTimeoutMs, int totalMs, int maxBytes)
        {
            HttpResult res = new HttpResult();
            TcpClient client = null;
            SslStream ssl = null;
            try
            {
                client = new TcpClient();
                Stopwatch sw = Stopwatch.StartNew();
                IAsyncResult ar = client.BeginConnect(server, port, null, null);
                if (!ar.AsyncWaitHandle.WaitOne(connectTimeoutMs, false))
                    throw new Exception("TCP 连接超时(>" + connectTimeoutMs + "ms)");
                client.EndConnect(ar);
                res.TcpMs = sw.ElapsedMilliseconds;

                ssl = new SslStream(client.GetStream(), false, delegate { return true; });
                ssl.ReadTimeout = readTimeoutMs;
                ssl.WriteTimeout = readTimeoutMs;
                Handshake(ssl, server);
                res.TlsMs = sw.ElapsedMilliseconds - res.TcpMs;
                res.Protocol = ssl.SslProtocol.ToString();
                try { if (ssl.RemoteCertificate != null) res.Issuer = ssl.RemoteCertificate.Issuer; }
                catch { }

                StringBuilder sb = new StringBuilder();
                sb.Append(method + " " + path + " HTTP/1.1\r\n");
                sb.Append("Host: " + server + "\r\n");
                sb.Append("User-Agent: TraeNetDoctor/1.0\r\n");
                sb.Append("Accept: */*\r\n");
                if (headers != null)
                    foreach (KeyValuePair<string, string> kv in headers)
                        sb.Append(kv.Key + ": " + kv.Value + "\r\n");
                if (!string.IsNullOrEmpty(body))
                    sb.Append("Content-Length: " + Encoding.UTF8.GetByteCount(body) + "\r\n");
                sb.Append("Connection: close\r\n\r\n");
                if (!string.IsNullOrEmpty(body)) sb.Append(body);

                byte[] outBuf = Encoding.UTF8.GetBytes(sb.ToString());
                ssl.Write(outBuf, 0, outBuf.Length);

                Stopwatch ttfb = Stopwatch.StartNew();
                Stopwatch total = Stopwatch.StartNew();
                MemoryStream ms = new MemoryStream();
                byte[] buf = new byte[4096];
                bool first = true;
                while (total.ElapsedMilliseconds < totalMs && ms.Length < maxBytes)
                {
                    int n;
                    try { n = ssl.Read(buf, 0, buf.Length); }
                    catch { res.Stalls++; continue; }
                    if (n <= 0) break;
                    if (first) { res.TtfbMs = ttfb.ElapsedMilliseconds; first = false; }
                    ms.Write(buf, 0, n);
                }
                res.ReadMs = total.ElapsedMilliseconds;
                res.Bytes = (int)ms.Length;

                string raw = Encoding.UTF8.GetString(ms.ToArray());
                int idx = raw.IndexOf("\r\n\r\n");
                string head = idx >= 0 ? raw.Substring(0, idx) : raw;
                string bodyText = idx >= 0 ? raw.Substring(idx + 4) : "";
                int nl = head.IndexOf("\r\n");
                res.FirstLine = nl > 0 ? head.Substring(0, nl) : head;
                bodyText = System.Text.RegularExpressions.Regex.Replace(bodyText.Trim(), "\\s+", " ");
                if (bodyText.Length > 200) bodyText = bodyText.Substring(0, 200);
                res.Snippet = bodyText;
                res.Ok = true;
            }
            catch (Exception ex) { res.Error = ex.Message; }
            finally
            {
                if (ssl != null) try { ssl.Dispose(); } catch { }
                if (client != null) try { client.Close(); } catch { }
            }
            return res;
        }
    }

    static class Sys
    {
        public static bool IsAdmin()
        {
            try
            {
                using (WindowsIdentity id = WindowsIdentity.GetCurrent())
                    return new WindowsPrincipal(id).IsInRole(WindowsBuiltInRole.Administrator);
            }
            catch { return false; }
        }

        public static int ProxyEnable()
        {
            try
            {
                using (RegistryKey k = Registry.CurrentUser.OpenSubKey(@"Software\Microsoft\Windows\CurrentVersion\Internet Settings"))
                {
                    if (k == null) return -1;
                    object v = k.GetValue("ProxyEnable");
                    return v == null ? 0 : Convert.ToInt32(v);
                }
            }
            catch { return -1; }
        }

        public static string ProxyValue(string name)
        {
            try
            {
                using (RegistryKey k = Registry.CurrentUser.OpenSubKey(@"Software\Microsoft\Windows\CurrentVersion\Internet Settings"))
                {
                    if (k == null) return "";
                    object v = k.GetValue(name);
                    return v == null ? "" : v.ToString();
                }
            }
            catch { return ""; }
        }

        public static int WinsockLspCount()
        {
            try
            {
                using (RegistryKey k = Registry.LocalMachine.OpenSubKey(
                    @"SYSTEM\CurrentControlSet\Services\WinSock2\Parameters\Protocol_Catalog9\Catalog_Entries"))
                    return k == null ? -1 : k.SubKeyCount;
            }
            catch { return -1; }
        }

        public static string HostsPath()
        {
            return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System), @"drivers\etc\hosts");
        }

        public static List<string> HostsActiveLines()
        {
            List<string> list = new List<string>();
            try
            {
                if (!File.Exists(HostsPath())) return list;
                foreach (string line in File.ReadAllLines(HostsPath(), Encoding.Default))
                {
                    string t = line.Trim();
                    if (t.Length > 0 && !t.StartsWith("#")) list.Add(t);
                }
            }
            catch { }
            return list;
        }

        // 运行原生命令并返回输出。
        // 先显式把控制台代码页切成 UTF-8，再按 UTF-8 解码，
        // 否则 netsh 的中文输出会被按错误的代码页解码成乱码。
        public static string RunCmd(string command)
        {
            try
            {
                ProcessStartInfo psi = new ProcessStartInfo("cmd.exe", "/c chcp 65001>nul & " + command + " 2>&1");
                psi.UseShellExecute = false;
                psi.RedirectStandardOutput = true;
                psi.CreateNoWindow = true;
                psi.StandardOutputEncoding = Encoding.UTF8;
                using (Process p = Process.Start(psi))
                {
                    string o = p.StandardOutput.ReadToEnd();
                    p.WaitForExit(30000);
                    return o.TrimEnd();
                }
            }
            catch (Exception ex) { return "[error] " + ex.Message; }
        }

        // .NET 的 IPv4InterfaceProperties.Mtu 在 Win10 上会给出偏差值
        // （实测 WLAN 真实 1500 却报 1400），所以改用系统网络栈自己的数据源
        // MSFT_NetIPInterface，取值与 netsh / Get-NetIPInterface 一致。
        public static Dictionary<int, long> GetRealMtu()
        {
            Dictionary<int, long> map = new Dictionary<int, long>();
            try
            {
                ManagementObjectSearcher searcher = new ManagementObjectSearcher(
                    "root\\StandardCimv2",
                    "SELECT InterfaceIndex, NlMtu FROM MSFT_NetIPInterface WHERE AddressFamily=2");
                foreach (ManagementBaseObject mo in searcher.Get())
                {
                    try
                    {
                        int idx = Convert.ToInt32(mo["InterfaceIndex"]);
                        long mtu = Convert.ToInt64(mo["NlMtu"]);
                        if (!map.ContainsKey(idx)) map[idx] = mtu;
                    }
                    catch { }
                }
            }
            catch { }
            return map;
        }
    }

    // =======================================================================
    //  二、修复执行（以管理员身份运行的子进程）
    // =======================================================================
    static class RepairRunner
    {
        static StringBuilder _sb = new StringBuilder();

        static void L(string s) { _sb.AppendLine(s); }

        static void Native(string title, string cmd)
        {
            L("");
            L("===== " + title + " =====");
            L("> " + cmd);
            try
            {
                ProcessStartInfo psi = new ProcessStartInfo("cmd.exe", "/c chcp 65001>nul & " + cmd + " 2>&1");
                psi.UseShellExecute = false;
                psi.RedirectStandardOutput = true;
                psi.CreateNoWindow = true;
                psi.StandardOutputEncoding = Encoding.UTF8;
                using (Process p = Process.Start(psi))
                {
                    string o = p.StandardOutput.ReadToEnd();
                    p.WaitForExit(60000);
                    L(o.TrimEnd());
                    L("[exit code] " + p.ExitCode);
                }
            }
            catch (Exception ex) { L("[error] " + ex.Message); }
        }

        static void WinInet()
        {
            L("");
            L("===== 关闭残留的用户级系统代理 (WinINET) =====");
            try
            {
                using (RegistryKey k = Registry.CurrentUser.OpenSubKey(
                    @"Software\Microsoft\Windows\CurrentVersion\Internet Settings", true))
                {
                    if (k == null) { L("  无法打开注册表项"); return; }
                    object en = k.GetValue("ProxyEnable");
                    L("  before: ProxyEnable=" + (en == null ? "(空)" : en.ToString())
                        + "  ProxyServer=" + Convert.ToString(k.GetValue("ProxyServer"))
                        + "  AutoConfigURL=" + Convert.ToString(k.GetValue("AutoConfigURL")));
                    if (en == null || Convert.ToInt32(en) != 0)
                    {
                        k.SetValue("ProxyEnable", 0, RegistryValueKind.DWord);
                        L("  after : ProxyEnable=0 (ProxyServer 值保留，需要时可手动还原)");
                    }
                    else L("  after : 系统代理本就未启用，无需处理");
                }
            }
            catch (Exception ex) { L("  [error] " + ex.Message); }
        }

        static void Hosts()
        {
            L("");
            L("===== 清理 HOSTS 中的 Trae / DeepSeek 条目 =====");
            string hosts = Sys.HostsPath();
            if (!File.Exists(hosts)) { L("  未找到 HOSTS 文件，跳过"); return; }
            string backup = hosts + ".trae_net_doctor.bak";
            try
            {
                File.Copy(hosts, backup, true);
                L("  已备份到: " + backup);
            }
            catch (Exception ex) { L("  [error] 备份失败: " + ex.Message); return; }

            List<string> kept = new List<string>();
            int hit = 0;
            try
            {
                foreach (string line in File.ReadAllLines(hosts, Encoding.Default))
                {
                    string t = line.Trim();
                    if (t.Length > 0 && !t.StartsWith("#") &&
                        (t.IndexOf("trae", StringComparison.OrdinalIgnoreCase) >= 0 ||
                         t.IndexOf("deepseek", StringComparison.OrdinalIgnoreCase) >= 0))
                    {
                        kept.Add("# [TraeNetDoctor] " + line);
                        L("  已注释: " + line);
                        hit++;
                    }
                    else kept.Add(line);
                }
                if (hit > 0)
                {
                    File.WriteAllLines(hosts, kept.ToArray(), Encoding.Default);
                    L("  共处理 " + hit + " 条可疑条目");
                }
                else L("  未发现相关条目，无需处理");
            }
            catch (Exception ex) { L("  [error] " + ex.Message); }
        }

        static void Mtu(int ifIndex, int mtu)
        {
            L("");
            L("===== 调整网卡 MTU (接口索引 " + ifIndex + " -> " + mtu + ") =====");
            Native("修改 IPv4 MTU", "netsh interface ipv4 set subinterface " + ifIndex + " mtu=" + mtu + " store=persistent");
            Native("修改 IPv6 MTU", "netsh interface ipv6 set subinterface " + ifIndex + " mtu=" + mtu + " store=persistent");
        }

        public static void Run(string tasks, string logPath)
        {
            L("修复开始 : " + DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss"));
            L("管理员权限: " + Sys.IsAdmin());
            L("任务列表 : " + tasks);
            foreach (string raw in tasks.Split(new char[] { ',' }, StringSplitOptions.RemoveEmptyEntries))
            {
                string task = raw.Trim();
                switch (task)
                {
                    case "flushdns": Native("刷新 DNS 解析缓存", "ipconfig /flushdns"); break;
                    case "winhttp":
                        Native("修复前的 WinHTTP 代理配置", "netsh winhttp show proxy");
                        Native("重置 WinHTTP 代理为直连", "netsh winhttp reset proxy");
                        break;
                    case "wininet": WinInet(); break;
                    case "autotuning": Native("恢复 TCP 接收窗口自动调优", "netsh int tcp set global autotuninglevel=normal"); break;
                    case "winsock": Native("重置 Winsock 目录（清除 LSP 注入，需重启）", "netsh winsock reset"); break;
                    case "tcpip": Native("重置 TCP/IP 协议栈（需重启）", "netsh int ip reset"); break;
                    case "hosts": Hosts(); break;
                    default:
                        if (task.StartsWith("mtu:"))
                        {
                            string[] seg = task.Split(':');
                            if (seg.Length == 3) Mtu(int.Parse(seg[1]), int.Parse(seg[2]));
                        }
                        else L("[skip] 未知任务: " + task);
                        break;
                }
            }
            L("");
            L("修复结束 : " + DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss"));
            L("注意：重置 Winsock / TCP-IP 后必须重启电脑才会生效。");
            if (!string.IsNullOrEmpty(logPath))
            {
                try { File.WriteAllText(logPath, _sb.ToString(), new UTF8Encoding(true)); }
                catch { }
            }
        }
    }

    // =======================================================================
    //  三、界面
    // =======================================================================
    class ItemRow
    {
        public Panel Box;
        public Label Bar, Status, Name, Ms, Detail;
        public Color Normal = Color.FromArgb(0, 130, 0);
        public Color Warn = Color.FromArgb(200, 110, 0);
        public Color Fail = Color.FromArgb(200, 0, 0);
        public Color Idle = Color.FromArgb(150, 150, 150);

        public static ItemRow Create(string name)
        {
            ItemRow r = new ItemRow();
            r.Box = new Panel();
            r.Box.Height = 27;
            r.Box.Margin = new Padding(0, 0, 0, 1);
            r.Box.BackColor = Color.White;

            r.Detail = new Label();
            r.Detail.Dock = DockStyle.Fill;
            r.Detail.TextAlign = ContentAlignment.MiddleLeft;
            r.Detail.AutoEllipsis = true;
            r.Detail.Text = "待检测";
            r.Detail.ForeColor = Color.Gray;

            r.Ms = new Label();
            r.Ms.Dock = DockStyle.Left;
            r.Ms.Width = 86;
            r.Ms.TextAlign = ContentAlignment.MiddleRight;
            r.Ms.ForeColor = Color.Gray;

            r.Name = new Label();
            r.Name.Dock = DockStyle.Left;
            r.Name.Width = 330;
            r.Name.TextAlign = ContentAlignment.MiddleLeft;
            r.Name.Text = name;

            r.Status = new Label();
            r.Status.Dock = DockStyle.Left;
            r.Status.Width = 66;
            r.Status.TextAlign = ContentAlignment.MiddleCenter;
            r.Status.Font = new Font("Microsoft YaHei UI", 9F, FontStyle.Bold);
            r.Status.Text = "待检测";
            r.Status.ForeColor = r.Idle;

            r.Bar = new Label();
            r.Bar.Dock = DockStyle.Left;
            r.Bar.Width = 5;
            r.Bar.BackColor = Color.Gainsboro;

            // 停靠顺序：索引越大越先停靠（越靠左），索引 0 最后停靠并占满剩余宽度
            r.Box.Controls.Add(r.Detail);
            r.Box.Controls.Add(r.Ms);
            r.Box.Controls.Add(r.Name);
            r.Box.Controls.Add(r.Status);
            r.Box.Controls.Add(r.Bar);
            return r;
        }

        public void Reset()
        {
            Status.Text = "待检测";
            Status.ForeColor = Idle;
            Bar.BackColor = Color.Gainsboro;
            Ms.Text = "";
            Detail.Text = "待检测";
            Detail.ForeColor = Color.Gray;
        }

        public void Set(string status, long ms, string detail)
        {
            Color c = Idle;
            if (status == "正常") c = Normal;
            else if (status == "警告") c = Warn;
            else if (status == "失败") c = Fail;

            Status.Text = status;
            Status.ForeColor = c;
            Bar.BackColor = c;
            Ms.Text = ms >= 0 ? ms + " ms" : "";
            Detail.Text = detail;
            Detail.ForeColor = (status == "失败") ? Fail : Color.FromArgb(50, 50, 50);
        }
    }

    class MainForm : Form
    {
        TextBox _txtTargets, _txtKey, _txtFixLog;
        Button _btnDetect, _btnCopy, _btnExport, _btnRefreshCfg, _btnDoRepair;
        Label _lblStatus, _lblSummary;
        FlowLayoutPanel _flowCheck, _flowCfg;
        TabControl _tabs;
        CheckedListBox _chkFix;
        ComboBox _cmbIf;
        NumericUpDown _numMtu;
        CheckBox _chkMtu;

        Dictionary<string, ItemRow> _map = new Dictionary<string, ItemRow>();
        List<ItemRow> _checkRows = new List<ItemRow>();
        List<string> _targets = new List<string>();
        volatile bool _running;

        // ---- 接口监控（连接级，实时事件流）----
        // 原理：轮询本机 TCP 连接表（无需管理员、无需驱动），
        // 把远端为被监控域名 IP 且端口 443 的连接挑出来，
        // 逐个跟踪它的生命周期：新建 / 状态变化 / 结束 / 异常。
        ListView _lstMon;
        Button _btnMon;
        Label _lblMonStatus;
        CheckBox _chkMonScroll;
        System.Windows.Forms.Timer _monTimer;
        readonly Dictionary<string, MonConn> _monAlive = new Dictionary<string, MonConn>();
        Dictionary<string, string> _monIpHits = new Dictionary<string, string>();
        DateTime _monDnsTime = DateTime.MinValue;
        bool _monRunning;
        volatile bool _monDnsBusy;
        int _monEventCount;
        const int MonSynAlarmSec = 5;      // SYN 发出后多少秒还没回应就报异常
        const int MonMaxRows = 3000;       // 事件列表上限，超出丢弃最旧的
        static readonly string[] MonHosts = new string[] { "api.deepseek.com", "api.trae.ai", "www.trae.cn" };

        class MonConn
        {
            public string Key;
            public string Host;
            public string RemoteIp;
            public int LocalPort;
            public DateTime FirstSeen = DateTime.Now;
            public TcpState LastState;
            public bool SynAlarmed;
            public bool Self;
        }

        public MainForm()
        {
            Text = "Trae / DeepSeek 通路诊断与修复工具";
            Size = new Size(1220, 830);
            MinimumSize = new Size(1000, 700);
            StartPosition = FormStartPosition.CenterScreen;
            Font = new Font("Microsoft YaHei UI", 9F);

            TableLayoutPanel root = new TableLayoutPanel();
            root.Dock = DockStyle.Fill;
            root.ColumnCount = 1;
            root.RowCount = 3;
            root.RowStyles.Add(new RowStyle(SizeType.Absolute, 116));
            root.RowStyles.Add(new RowStyle(SizeType.Percent, 100));
            root.RowStyles.Add(new RowStyle(SizeType.Absolute, 30));
            Controls.Add(root);

            // ---------- 顶部输入区 ----------
            TableLayoutPanel top = new TableLayoutPanel();
            top.Dock = DockStyle.Fill;
            top.ColumnCount = 1;
            top.RowCount = 3;
            top.RowStyles.Add(new RowStyle(SizeType.Absolute, 36));
            top.RowStyles.Add(new RowStyle(SizeType.Absolute, 36));
            top.RowStyles.Add(new RowStyle(SizeType.Absolute, 42));
            top.Padding = new Padding(10, 8, 10, 0);

            TableLayoutPanel r1 = new TableLayoutPanel();
            r1.Dock = DockStyle.Fill;
            r1.ColumnCount = 4;
            r1.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 86));
            r1.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
            r1.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 78));
            r1.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 130));
            Label l1 = new Label();
            l1.Text = "检测目标:";
            l1.Dock = DockStyle.Fill;
            l1.TextAlign = ContentAlignment.MiddleLeft;
            _txtTargets = new TextBox();
            _txtTargets.Dock = DockStyle.Fill;
            _txtTargets.Text = "api.deepseek.com, api.trae.ai, www.trae.cn";
            Label l2 = new Label();
            l2.Text = "DeepSeek Key:";
            l2.Dock = DockStyle.Fill;
            l2.TextAlign = ContentAlignment.MiddleLeft;
            _txtKey = new TextBox();
            _txtKey.Dock = DockStyle.Fill;
            _txtKey.UseSystemPasswordChar = true;
            r1.Controls.Add(l1, 0, 0);
            r1.Controls.Add(_txtTargets, 1, 0);
            r1.Controls.Add(l2, 2, 0);
            r1.Controls.Add(_txtKey, 3, 0);

            TableLayoutPanel r2 = new TableLayoutPanel();
            r2.Dock = DockStyle.Fill;
            r2.ColumnCount = 3;
            r2.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 86));
            r2.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
            r2.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 470));
            Label l3 = new Label();
            l3.Text = "目标可改，英文逗号分隔";
            l3.Dock = DockStyle.Fill;
            l3.TextAlign = ContentAlignment.MiddleLeft;
            l3.ForeColor = Color.Gray;
            Label l4 = new Label();
            l4.Dock = DockStyle.Fill;
            l4.TextAlign = ContentAlignment.MiddleLeft;
            l4.ForeColor = Color.Gray;
            l4.Text = "Key 仅内存使用不落盘；填了才能实测流式首 token 耗时";
            r2.Controls.Add(l3, 0, 0);
            r2.Controls.Add(new Label(), 1, 0);
            r2.Controls.Add(l4, 2, 0);

            FlowLayoutPanel r3 = new FlowLayoutPanel();
            r3.Dock = DockStyle.Fill;
            r3.FlowDirection = FlowDirection.LeftToRight;
            _btnDetect = MakeButton("开始检测", 120, true);
            _btnDetect.Click += delegate { StartDetection(); };
            Button btnFixTab = MakeButton("一键修复", 110, true);
            btnFixTab.Click += delegate { _tabs.SelectedIndex = 3; };
            _btnCopy = MakeButton("复制报告", 100, false);
            _btnCopy.Click += delegate { CopyReport(); };
            _btnExport = MakeButton("导出报告", 100, false);
            _btnExport.Click += delegate { ExportReport(); };
            r3.Controls.Add(_btnDetect);
            r3.Controls.Add(btnFixTab);
            r3.Controls.Add(_btnCopy);
            r3.Controls.Add(_btnExport);

            top.Controls.Add(r1, 0, 0);
            top.Controls.Add(r2, 0, 1);
            top.Controls.Add(r3, 0, 2);
            root.Controls.Add(top, 0, 0);

            // ---------- 底部状态条 ----------
            _lblStatus = new Label();
            _lblStatus.Dock = DockStyle.Fill;
            _lblStatus.TextAlign = ContentAlignment.MiddleLeft;
            _lblStatus.Padding = new Padding(10, 0, 0, 0);
            _lblStatus.BackColor = Color.FromArgb(240, 240, 240);
            _lblStatus.Text = "就绪。点【开始检测】对全部项目逐条检测。";
            root.Controls.Add(_lblStatus, 0, 2);

            // ---------- 选项卡 ----------
            _tabs = new TabControl();
            _tabs.Dock = DockStyle.Fill;
            _tabs.Font = new Font("Microsoft YaHei UI", 9.5F);
            root.Controls.Add(_tabs, 0, 1);

            // === 检测页 ===
            TabPage tpCheck = new TabPage("  通路检测  ");
            tpCheck.Padding = new Padding(0);
            _flowCheck = NewFlow();
            Panel legend = new Panel();
            legend.Dock = DockStyle.Top;
            legend.Height = 26;
            legend.BackColor = Color.FromArgb(247, 247, 247);
            Label lg = new Label();
            lg.Dock = DockStyle.Fill;
            lg.TextAlign = ContentAlignment.MiddleLeft;
            lg.Padding = new Padding(10, 0, 0, 0);
            lg.Text = "全部检测项目打开即列出，检测时逐条刷新：  绿色=正常   橙色=可疑   红色=故障       每个目标会连做 5 次 TLS 握手，用来抓“时好时坏”的间歇性故障（ping 通却一直思考中，多半卡在这里）";
            lg.ForeColor = Color.FromArgb(90, 90, 90);
            legend.Controls.Add(lg);
            _lblSummary = new Label();
            _lblSummary.Dock = DockStyle.Top;
            _lblSummary.Height = 28;
            _lblSummary.TextAlign = ContentAlignment.MiddleLeft;
            _lblSummary.Padding = new Padding(10, 0, 0, 0);
            _lblSummary.Font = new Font("Microsoft YaHei UI", 9.5F, FontStyle.Bold);
            _lblSummary.Text = "尚未检测";
            // 停靠顺序：索引 0 最后停靠占满剩余
            tpCheck.Controls.Add(_flowCheck);
            tpCheck.Controls.Add(_lblSummary);
            tpCheck.Controls.Add(legend);
            _tabs.TabPages.Add(tpCheck);

            // === 接口监控页 ===
            TabPage tpMon = new TabPage("  接口监控  ");
            tpMon.Padding = new Padding(0);

            Panel monLegend = new Panel();
            monLegend.Dock = DockStyle.Top;
            monLegend.Height = 60;
            monLegend.BackColor = Color.FromArgb(247, 247, 247);
            Label mlg = new Label();
            mlg.Dock = DockStyle.Fill;
            mlg.Padding = new Padding(10, 2, 10, 2);
            mlg.Text =
                "请求卡住时看这里 ——  ① 列表里完全没有出现目标连接 = 请求压根没发出去（IDE 侧没发起）    ② 出现 SynSent 卡住或消失 = 请求发出去了但对端没回    ③ 出现 Established = 连接已建立，卡顿在应用层\r\n" +
                "注意：本工具自身检测产生的流量会标注「本工具检测」；连接表看不出已建立连接上是否真有数据在流动（那需要字节级模式）。监控目标固定为 api.deepseek.com / api.trae.ai / www.trae.cn";
            mlg.ForeColor = Color.FromArgb(90, 90, 90);
            monLegend.Controls.Add(mlg);

            Panel monBar = new Panel();
            monBar.Dock = DockStyle.Top;
            monBar.Height = 34;
            _btnMon = MakeButton("停止监控", 100, true);
            _btnMon.Location = new Point(8, 2);
            _btnMon.Click += delegate { ToggleMonitor(); };
            _chkMonScroll = new CheckBox();
            _chkMonScroll.Text = "自动滚动";
            _chkMonScroll.Checked = true;
            _chkMonScroll.Location = new Point(120, 8);
            _chkMonScroll.Width = 90;
            Button btnMonClear = MakeButton("清空", 70, false);
            btnMonClear.Location = new Point(215, 2);
            btnMonClear.Click += delegate { _lstMon.Items.Clear(); _monEventCount = 0; UpdateMonStatus(0); };
            _lblMonStatus = new Label();
            _lblMonStatus.AutoSize = false;
            _lblMonStatus.Location = new Point(295, 8);
            _lblMonStatus.Size = new Size(900, 20);
            _lblMonStatus.Anchor = AnchorStyles.Top | AnchorStyles.Left | AnchorStyles.Right;
            _lblMonStatus.TextAlign = ContentAlignment.MiddleLeft;
            _lblMonStatus.ForeColor = Color.FromArgb(60, 60, 60);
            _lblMonStatus.Text = "未启动";
            monBar.Controls.Add(_btnMon);
            monBar.Controls.Add(_chkMonScroll);
            monBar.Controls.Add(btnMonClear);
            monBar.Controls.Add(_lblMonStatus);

            _lstMon = new ListView();
            _lstMon.Dock = DockStyle.Fill;
            _lstMon.View = View.Details;
            _lstMon.FullRowSelect = true;
            _lstMon.GridLines = true;
            _lstMon.HideSelection = false;
            _lstMon.Font = new Font("Microsoft YaHei UI", 9F);
            _lstMon.Columns.Add("时间", 85);
            _lstMon.Columns.Add("事件", 55);
            _lstMon.Columns.Add("目标", 140);
            _lstMon.Columns.Add("远端 IP", 135);
            _lstMon.Columns.Add("本地端口", 70);
            _lstMon.Columns.Add("状态", 135);
            _lstMon.Columns.Add("存活", 70);
            _lstMon.Columns.Add("说明", 470);

            tpMon.Controls.Add(_lstMon);
            tpMon.Controls.Add(monLegend);
            tpMon.Controls.Add(monBar);
            _tabs.TabPages.Add(tpMon);

            // === 配置页 ===
            TabPage tpCfg = new TabPage("  系统配置  ");
            _flowCfg = NewFlow();
            Panel cfgBar = new Panel();
            cfgBar.Dock = DockStyle.Top;
            cfgBar.Height = 34;
            _btnRefreshCfg = MakeButton("刷新配置", 110, false);
            _btnRefreshCfg.Location = new Point(8, 4);
            _btnRefreshCfg.Click += delegate { BuildConfigRows(); };
            cfgBar.Controls.Add(_btnRefreshCfg);
            tpCfg.Controls.Add(_flowCfg);
            tpCfg.Controls.Add(cfgBar);
            _tabs.TabPages.Add(tpCfg);

            // === 修复页 ===
            TabPage tpFix = new TabPage("  一键修复  ");
            tpFix.Padding = new Padding(8);
            _txtFixLog = new TextBox();
            _txtFixLog.Dock = DockStyle.Fill;
            _txtFixLog.Multiline = true;
            _txtFixLog.ReadOnly = true;
            _txtFixLog.ScrollBars = ScrollBars.Both;
            _txtFixLog.WordWrap = false;
            _txtFixLog.BackColor = Color.White;
            _txtFixLog.Font = new Font("Consolas", 9F);

            FlowLayoutPanel fixBtn = new FlowLayoutPanel();
            fixBtn.Dock = DockStyle.Top;
            fixBtn.Height = 44;
            _btnDoRepair = MakeButton("执行修复（需管理员）", 190, true);
            _btnDoRepair.Click += delegate { DoRepair(); };
            Button bHosts = MakeButton("打开 HOSTS", 110, false);
            bHosts.Click += delegate { try { Process.Start("notepad.exe", Sys.HostsPath()); } catch { } };
            Button bNet = MakeButton("打开网络设置", 120, false);
            bNet.Click += delegate { try { Process.Start("ncpa.cpl"); } catch { } };
            fixBtn.Controls.Add(_btnDoRepair);
            fixBtn.Controls.Add(bHosts);
            fixBtn.Controls.Add(bNet);

            TableLayoutPanel mtuRow = new TableLayoutPanel();
            mtuRow.Dock = DockStyle.Top;
            mtuRow.Height = 38;
            mtuRow.ColumnCount = 5;
            mtuRow.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 300));
            mtuRow.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 60));
            mtuRow.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 380));
            mtuRow.ColumnStyles.Add(new ColumnStyle(SizeType.Absolute, 90));
            mtuRow.ColumnStyles.Add(new ColumnStyle(SizeType.Percent, 100));
            _chkMtu = new CheckBox();
            _chkMtu.Text = "调整网卡 MTU（修复大包黑洞）";
            _chkMtu.Dock = DockStyle.Fill;
            Label lm1 = new Label();
            lm1.Text = "MTU 值:";
            lm1.Dock = DockStyle.Fill;
            lm1.TextAlign = ContentAlignment.MiddleLeft;
            _cmbIf = new ComboBox();
            _cmbIf.Dock = DockStyle.Fill;
            _cmbIf.DropDownStyle = ComboBoxStyle.DropDownList;
            _numMtu = new NumericUpDown();
            _numMtu.Dock = DockStyle.Fill;
            _numMtu.Minimum = 576;
            _numMtu.Maximum = 1500;
            _numMtu.Value = 1400;
            Label lm2 = new Label();
            lm2.Text = "默认 1400 可绕过绝大多数大包黑洞";
            lm2.Dock = DockStyle.Fill;
            lm2.TextAlign = ContentAlignment.MiddleLeft;
            lm2.ForeColor = Color.Gray;
            mtuRow.Controls.Add(_chkMtu, 0, 0);
            mtuRow.Controls.Add(lm1, 1, 0);
            mtuRow.Controls.Add(_cmbIf, 2, 0);
            mtuRow.Controls.Add(_numMtu, 3, 0);
            mtuRow.Controls.Add(lm2, 4, 0);

            _chkFix = new CheckedListBox();
            _chkFix.Dock = DockStyle.Top;
            _chkFix.Height = 172;
            _chkFix.CheckOnClick = true;
            string[] fixItems = new string[]
            {
                "flushdns|刷新 DNS 解析缓存",
                "winhttp|重置 WinHTTP 系统代理为直连",
                "wininet|关闭残留的用户级系统代理 (ProxyEnable=0)",
                "autotuning|恢复 TCP 接收窗口自动调优 (autotuninglevel=normal)",
                "winsock|重置 Winsock 目录，清除 LSP 注入  ※需重启",
                "hosts|清理 HOSTS 中的 Trae / DeepSeek 条目（自动备份）",
                "tcpip|重置 TCP/IP 协议栈  ※需重启，静态 IP 机器慎用"
            };
            foreach (string s in fixItems) _chkFix.Items.Add(s, true);
            _chkFix.SetItemChecked(6, false);

            Label fixInfo = new Label();
            fixInfo.Dock = DockStyle.Top;
            fixInfo.Height = 54;
            fixInfo.TextAlign = ContentAlignment.MiddleLeft;
            fixInfo.ForeColor = Color.FromArgb(80, 80, 80);
            fixInfo.Text = "勾选后点【执行修复】，会弹出 UAC 授权框（修复必须管理员权限）。\r\n重置 Winsock / TCP-IP 需重启生效；HOSTS 与 MTU 修改前都会自动留备份。";

            tpFix.Controls.Add(_txtFixLog);
            tpFix.Controls.Add(fixBtn);
            tpFix.Controls.Add(mtuRow);
            tpFix.Controls.Add(_chkFix);
            tpFix.Controls.Add(fixInfo);
            _tabs.TabPages.Add(tpFix);

            Load += delegate
            {
                BuildCheckRows();
                BuildConfigRows();
                FillInterfaces();
                StartMonitor();     // 打开软件即持续监控
            };
            Resize += delegate { ResizeRows(_flowCheck); ResizeRows(_flowCfg); };
        }

        // ------------------------------------------------------------------
        static Button MakeButton(string text, int width, bool bold)
        {
            Button b = new Button();
            b.Text = text;
            b.Width = width;
            b.Height = 32;
            b.Margin = new Padding(0, 0, 8, 0);
            b.Font = new Font("Microsoft YaHei UI", 9.5F, bold ? FontStyle.Bold : FontStyle.Regular);
            return b;
        }

        static FlowLayoutPanel NewFlow()
        {
            FlowLayoutPanel f = new FlowLayoutPanel();
            f.Dock = DockStyle.Fill;
            f.FlowDirection = FlowDirection.TopDown;
            f.WrapContents = false;
            f.AutoScroll = true;
            f.BackColor = Color.FromArgb(250, 250, 250);
            f.Padding = new Padding(6, 4, 0, 0);
            return f;
        }

        static void ResizeRows(FlowLayoutPanel flow)
        {
            if (flow == null) return;
            int w = flow.ClientSize.Width - SystemInformation.VerticalScrollBarWidth - 14;
            if (w < 300) w = 300;
            foreach (Control c in flow.Controls) if (c is Panel) c.Width = w;
        }

        void SetStatus(string t)
        {
            if (InvokeRequired) { BeginInvoke(new Action<string>(SetStatus), t); return; }
            _lblStatus.Text = t;
        }

        // ------------------------------------------------------------------
        //  检测项目录：全部预先生成，打开就能看到每一条
        // ------------------------------------------------------------------
        static readonly string[][] FixedItems = new string[][]
        {
            new string[]{ "1",  "1. 本机 IPv4 地址" },
            new string[]{ "2",  "2. 默认路由 / 网关地址" },
            new string[]{ "3",  "3. 网关连通 (ICMP)" },
            new string[]{ "4",  "4. 公网连通 (ICMP 223.5.5.5)" },
            new string[]{ "5",  "5. 公共 DNS 解析 (www.baidu.com)" },
            new string[]{ "6",  "6. 本机 DNS 服务器" },
            new string[]{ "7",  "7. 系统代理 (WinINET)" },
            new string[]{ "8",  "8. HOSTS 干扰" },
            new string[]{ "9",  "9. 路径 MTU（大包黑洞）" },
            new string[]{ "10", "10. 出口网卡类型" },
            new string[]{ "11", "11. 多网卡同时启用" },
            new string[]{ "12", "12. 本地代理端口" },
            new string[]{ "13", "13. Winsock LSP 条目数" }
        };

        void BuildCheckRows()
        {
            _flowCheck.SuspendLayout();
            _flowCheck.Controls.Clear();
            _checkRows.Clear();
            _map.Clear();

            foreach (string[] it in FixedItems) AddCheckRow(it[0], it[1]);

            _targets = ParseTargets(_txtTargets.Text);
            foreach (string t in _targets)
            {
                AddCheckRow("t0:" + t, "14. DNS 解析  " + t);
                AddCheckRow("t1:" + t, "14. TCP 443  " + t);
                AddCheckRow("t5:" + t, "14. 边缘 IP 逐个握手  " + t);
                AddCheckRow("t4:" + t, "14. TLS 握手稳定性 (5 次)  " + t);
                AddCheckRow("t2:" + t, "14. TLS 握手 / 证书  " + t);
                AddCheckRow("t3:" + t, "14. HTTPS 首字节  " + t);
            }
            AddCheckRow("ds1", "15. DeepSeek 接口连通 (POST /chat/completions)");
            AddCheckRow("ds2", "15. DeepSeek 流式首 token 耗时");

            _flowCheck.ResumeLayout();
            ResizeRows(_flowCheck);
        }

        void AddCheckRow(string key, string name)
        {
            ItemRow r = ItemRow.Create(name);
            _flowCheck.Controls.Add(r.Box);
            _checkRows.Add(r);
            _map[key] = r;
        }

        static List<string> ParseTargets(string raw)
        {
            List<string> list = new List<string>();
            if (raw == null) return list;
            foreach (string s in raw.Split(new char[] { ',', ';', ' ', '|', '\t' }, StringSplitOptions.RemoveEmptyEntries))
            {
                string t = s.Trim();
                if (t.IndexOf('.') >= 0 && !list.Contains(t)) list.Add(t);
            }
            return list;
        }

        void SetRow(string key, string status, long ms, string detail)
        {
            if (InvokeRequired) { BeginInvoke(new Action<string, string, long, string>(SetRow), key, status, ms, detail); return; }
            ItemRow r;
            if (_map.TryGetValue(key, out r)) r.Set(status, ms, detail);
        }

        // ------------------------------------------------------------------
        //  检测主流程（后台线程）
        // ------------------------------------------------------------------
        void StartDetection()
        {
            if (_running) return;
            _running = true;
            _btnDetect.Enabled = false;
            _btnCopy.Enabled = false;
            _btnExport.Enabled = false;
            foreach (ItemRow r in _checkRows) r.Reset();
            _lblSummary.Text = "检测进行中...";
            _lblSummary.ForeColor = Color.FromArgb(60, 60, 60);
            _tabs.SelectedIndex = 0;

            BuildCheckRows();
            _txtTargets.Enabled = false;
            _txtKey.Enabled = false;

            string key = _txtKey.Text.Trim();
            List<string> targets = new List<string>(_targets);
            Thread th = new Thread(delegate () { Detect(targets, key); });
            th.IsBackground = true;
            th.Start();
        }

        void Detect(List<string> targets, string apiKey)
        {
            int fail = 0, warn = 0;
            try { fail = DoDetect(targets, apiKey, ref warn); }
            catch (Exception ex) { SetStatus("检测异常: " + ex.Message); }

            int total = _checkRows.Count;
            int ok = total - fail - warn;
            if (ok < 0) ok = 0;
            BeginInvoke(new Action(delegate
            {
                _lblSummary.Text = "检测完成：共 " + total + " 项  |  正常 " + ok + "  |  可疑 " + warn + "  |  故障 " + fail
                    + (fail > 0 ? "      <<< 故障项就是问题根源，看红色那几行" : (warn > 0 ? "      <<< 有可疑项，注意橙色行" : "      <<< 网络链路全部正常"));
                _lblSummary.ForeColor = fail > 0 ? Color.FromArgb(200, 0, 0)
                    : (warn > 0 ? Color.FromArgb(200, 110, 0) : Color.FromArgb(0, 130, 0));
                SetStatus("就绪。");
                _running = false;
                _btnDetect.Enabled = true;
                _btnCopy.Enabled = true;
                _btnExport.Enabled = true;
                _txtTargets.Enabled = true;
                _txtKey.Enabled = true;
            }));
        }

        int DoDetect(List<string> targets, string apiKey, ref int warn)
        {
            int fail = 0;
            NetworkInterface[] nics = NetworkInterface.GetAllNetworkInterfaces();

            // ---- 1. 本机 IPv4 ----
            SetStatus("检测 1/15：本机 IPv4 地址 ...");
            Stopwatch sw = Stopwatch.StartNew();
            List<string> myIps = new List<string>();
            List<string> apipa = new List<string>();
            List<int> nicIndexes = new List<int>();
            foreach (NetworkInterface ni in nics)
            {
                if (ni.OperationalStatus != OperationalStatus.Up) continue;
                if (ni.NetworkInterfaceType == NetworkInterfaceType.Loopback) continue;
                try
                {
                    IPInterfaceProperties ip = ni.GetIPProperties();
                    bool has = false;
                    foreach (UnicastIPAddressInformation ua in ip.UnicastAddresses)
                    {
                        if (ua.Address.AddressFamily != AddressFamily.InterNetwork) continue;
                        string s = ua.Address.ToString();
                        if (s.StartsWith("169.254.")) { apipa.Add("169.254.x [" + ni.Name + "]"); continue; }
                        myIps.Add(s + "/" + ua.PrefixLength + " [" + ni.Name + "]");
                        has = true;
                    }
                    if (has)
                    {
                        try { nicIndexes.Add(ip.GetIPv4Properties().Index); } catch { }
                    }
                }
                catch { }
            }
            if (myIps.Count > 0)
                SetRow("1", "正常", sw.ElapsedMilliseconds, string.Join("  ;  ", myIps.ToArray()));
            else
            {
                fail++;
                SetRow("1", "失败", sw.ElapsedMilliseconds, "没有任何可用的 IPv4 地址，网卡可能未连接");
            }

            // ---- 2. 默认路由 / 网关 ----
            SetStatus("检测 2/15：默认路由 ...");
            List<string> gws = new List<string>();
            string gateway = null;
            string ifAlias = null;
            foreach (NetworkInterface ni in nics)
            {
                if (ni.OperationalStatus != OperationalStatus.Up) continue;
                if (ni.NetworkInterfaceType == NetworkInterfaceType.Loopback) continue;
                try
                {
                    foreach (GatewayIPAddressInformation g in ni.GetIPProperties().GatewayAddresses)
                    {
                        if (g.Address.AddressFamily != AddressFamily.InterNetwork) continue;
                        if (gateway == null) { gateway = g.Address.ToString(); ifAlias = ni.Name; }
                        gws.Add(g.Address + " via " + ni.Name);
                    }
                }
                catch { }
            }
            if (gws.Count > 0)
                SetRow("2", "正常", 0, string.Join("  ;  ", gws.ToArray()));
            else
            {
                fail++;
                SetRow("2", "失败", 0, "没有默认网关，无法访问外网");
            }

            // ---- 3. 网关连通 ----
            SetStatus("检测 3/15：网关连通 ...");
            if (!string.IsNullOrEmpty(gateway))
            {
                PingReply pr = Net.Ping(gateway, 1500);
                if (pr != null && pr.Status == IPStatus.Success)
                    SetRow("3", "正常", pr.RoundtripTime, "RTT " + pr.RoundtripTime + " ms  (" + gateway + " via " + ifAlias + ")");
                else
                {
                    warn++;
                    SetRow("3", "警告", 0, "网关无 ICMP 响应（" + gateway + "，部分路由器禁 ping，不一定是故障）");
                }
            }
            else
            {
                warn++;
                SetRow("3", "警告", 0, "未取到网关地址");
            }

            // ---- 4. 公网 ICMP ----
            SetStatus("检测 4/15：公网连通 ...");
            PingReply p4 = Net.Ping("223.5.5.5", 2000);
            if (p4 != null && p4.Status == IPStatus.Success)
            {
                if (p4.RoundtripTime > 150) { warn++; SetRow("4", "警告", p4.RoundtripTime, "RTT 偏高 " + p4.RoundtripTime + " ms"); }
                else SetRow("4", "正常", p4.RoundtripTime, "RTT " + p4.RoundtripTime + " ms");
            }
            else
            {
                warn++;
                SetRow("4", "警告", 0, "ICMP 不通（可能被拦截），需结合下面 TCP/TLS 结果判断");
            }

            // ---- 5. 公共 DNS 解析 ----
            SetStatus("检测 5/15：DNS 解析 ...");
            sw = Stopwatch.StartNew();
            try
            {
                IPAddress[] addrs = Dns.GetHostAddresses("www.baidu.com");
                List<string> v4 = new List<string>();
                foreach (IPAddress a in addrs)
                    if (a.AddressFamily == AddressFamily.InterNetwork && !v4.Contains(a.ToString())) v4.Add(a.ToString());
                if (v4.Count > 0) SetRow("5", "正常", sw.ElapsedMilliseconds, string.Join(", ", v4.ToArray()));
                else { fail++; SetRow("5", "失败", sw.ElapsedMilliseconds, "没有返回 IPv4 地址"); }
            }
            catch (Exception ex) { fail++; SetRow("5", "失败", sw.ElapsedMilliseconds, ex.Message); }

            // ---- 6. DNS 服务器 ----
            SetStatus("检测 6/15：DNS 服务器 ...");
            List<string> dnsServers = new List<string>();
            foreach (NetworkInterface ni in nics)
            {
                if (ni.OperationalStatus != OperationalStatus.Up) continue;
                try
                {
                    foreach (IPAddress a in ni.GetIPProperties().DnsAddresses)
                        if (a.AddressFamily == AddressFamily.InterNetwork && !dnsServers.Contains(a.ToString()))
                            dnsServers.Add(a.ToString());
                }
                catch { }
            }
            string dnsTxt = dnsServers.Count > 0 ? string.Join(", ", dnsServers.ToArray()) : "(无)";
            if (dnsServers.Contains("127.0.0.1") || dnsServers.Contains("0.0.0.0"))
            {
                warn++;
                SetRow("6", "警告", 0, dnsTxt + "  本机代理 DNS，代理进程没运行就会直接解析失败");
            }
            else SetRow("6", "正常", 0, dnsTxt);

            // ---- 7. 系统代理 ----
            SetStatus("检测 7/15：系统代理 ...");
            int pe = Sys.ProxyEnable();
            string ps = Sys.ProxyValue("ProxyServer");
            string pac = Sys.ProxyValue("AutoConfigURL");
            if (pe == 1)
            {
                string ph = null; int pp = 0;
                if (!string.IsNullOrEmpty(ps))
                {
                    string proxyBody = ps;
                    int eq = proxyBody.IndexOf('=');
                    if (eq >= 0) proxyBody = proxyBody.Substring(eq + 1);
                    int cl = proxyBody.IndexOf(':');
                    if (cl > 0)
                    {
                        ph = proxyBody.Substring(0, cl);
                        int.TryParse(proxyBody.Substring(cl + 1).Trim(), out pp);
                    }
                }
                if (ph != null && pp > 0)
                {
                    if (Net.TcpPort(ph, pp, 1200))
                    {
                        warn++;
                        SetRow("7", "警告", 0, "代理已启用且端口可连: " + ps + "  代理不稳定会直接导致长连接卡死");
                    }
                    else
                    {
                        fail++;
                        SetRow("7", "失败", 0, "代理已启用但端口连不上: " + ps + "  这是‘发不出去 / 一直思考中’的直接原因");
                    }
                }
                else { warn++; SetRow("7", "警告", 0, "代理已启用: " + ps); }
            }
            else if (!string.IsNullOrEmpty(pac)) { warn++; SetRow("7", "警告", 0, "启用了 PAC 自动配置: " + pac); }
            else SetRow("7", "正常", 0, "未启用系统代理" + (string.IsNullOrEmpty(ps) ? "" : "（注册表残留 ProxyServer=" + ps + "，当前不生效）"));

            // ---- 8. HOSTS ----
            SetStatus("检测 8/15：HOSTS ...");
            List<string> hs = Sys.HostsActiveLines();
            List<string> suspect = new List<string>();
            foreach (string l in hs)
            {
                string low = l.ToLower();
                if (low.IndexOf("trae") >= 0 || low.IndexOf("deepseek") >= 0) suspect.Add(l);
            }
            if (suspect.Count > 0)
            {
                fail++;
                SetRow("8", "失败", 0, "发现 " + suspect.Count + " 条相关条目: " + string.Join(" | ", suspect.ToArray()));
            }
            else SetRow("8", "正常", 0, "无相关条目（自定义条目共 " + hs.Count + " 条）");

            // ---- 9. 路径 MTU ----
            SetStatus("检测 9/15：路径 MTU（大包黑洞）...");
            sw = Stopwatch.StartNew();
            int mtu = Net.PathMtu("223.5.5.5", 1200);
            if (mtu < 0) SetRow("9", "跳过", sw.ElapsedMilliseconds, "ICMP 不可用，无法探测（改看 HTTPS 首字节那一行）");
            else if (mtu >= 1492) SetRow("9", "正常", sw.ElapsedMilliseconds, "MTU " + mtu + "，大包可以通过");
            else if (mtu >= 1400) { warn++; SetRow("9", "警告", sw.ElapsedMilliseconds, "MTU " + mtu + "（偏小），TLS 大包可能被丢"); }
            else { fail++; SetRow("9", "失败", sw.ElapsedMilliseconds, "MTU 仅 " + mtu + " → 典型大包黑洞，ping 通但流式响应收不到"); }

            // ---- 10. 出口网卡类型 ----
            SetStatus("检测 10/15：出口网卡 ...");
            string vpnPattern = "tap|tun|vpn|clash|singbox|sing-box|zero ?tier|tailscale|wintun|wireguard|openvpn|radmin|vmware|virtualbox|hyper-v|npcap|虚拟|隧道";
            if (ifAlias != null && System.Text.RegularExpressions.Regex.IsMatch(ifAlias, vpnPattern, System.Text.RegularExpressions.RegexOptions.IgnoreCase))
            {
                warn++;
                SetRow("10", "警告", 0, "默认路由走虚拟/VPN 网卡: " + ifAlias + "  代理软件退出后残留路由会造成黑洞");
            }
            else if (ifAlias != null) SetRow("10", "正常", 0, "默认路由走物理网卡: " + ifAlias);
            else SetRow("10", "跳过", 0, "无默认路由");

            // ---- 11. 多网卡 ----
            SetStatus("检测 11/15：多网卡状态 ...");
            string ipTxt = string.Join("  ;  ", myIps.ToArray());
            if (myIps.Count > 1)
            {
                warn++;
                SetRow("11", "警告", 0, "同时有 " + myIps.Count + " 个可用地址: " + ipTxt + "  多宿主会让长连接绑定错误网卡，表现就是 ping 通但连接卡死");
            }
            else if (apipa.Count > 0)
            {
                warn++;
                SetRow("11", "警告", 0, "存在未取到 IP 的僵尸地址(169.254.x): " + string.Join(", ", apipa.ToArray()));
            }
            else SetRow("11", "正常", 0, ipTxt);

            // ---- 12. 本地代理端口 ----
            SetStatus("检测 12/15：本地代理端口 ...");
            int[] cand = new int[] { 7890, 7897, 7891, 1080, 10808, 10809, 8889, 2080 };
            List<string> listening = new List<string>();
            foreach (int p in cand) if (Net.TcpPort("127.0.0.1", p, 400)) listening.Add(p.ToString());
            if (listening.Count > 0)
            {
                string lt = string.Join(", ", listening.ToArray());
                if (pe == 1) { warn++; SetRow("12", "警告", 0, "端口 " + lt + " 在监听且系统代理已启用 → IDE 流量全走它，代理一抖动就一直思考中"); }
                else SetRow("12", "正常", 0, "端口 " + lt + " 在监听；系统代理已关闭" + (string.IsNullOrEmpty(ps) ? "" : "，残留值 " + ps + " 当前不生效"));
            }
            else
            {
                if (pe == 1) { fail++; SetRow("12", "失败", 0, "系统代理已启用，但没有任何本地代理端口在监听 → 请求必然全部卡死"); }
                else SetRow("12", "正常", 0, "未检测到常见本地代理端口 (7890/7897/1080/10809...)");
            }

            // ---- 13. Winsock LSP ----
            SetStatus("检测 13/15：Winsock LSP ...");
            int lsp = Sys.WinsockLspCount();
            if (lsp < 0) SetRow("13", "跳过", 0, "读取失败");
            else if (lsp > 20) { warn++; SetRow("13", "警告", 0, "条目数 " + lsp + "（偏多），可能有第三方 LSP 注入"); }
            else SetRow("13", "正常", 0, "条目数 " + lsp + "（原生 Windows 一般 10~14）");

            // ---- 14. 逐目标全链路 ----
            foreach (string t in targets)
            {
                // DNS
                SetStatus("检测：解析 " + t + " ...");
                sw = Stopwatch.StartNew();
                List<string> resolved = new List<string>();
                try
                {
                    foreach (IPAddress a in Dns.GetHostAddresses(t))
                        if (a.AddressFamily == AddressFamily.InterNetwork && !resolved.Contains(a.ToString()))
                            resolved.Add(a.ToString());
                }
                catch { }
                if (resolved.Count == 0)
                {
                    fail++;
                    SetRow("t0:" + t, "失败", sw.ElapsedMilliseconds, "解析不到 IP（DNS 污染 / 代理未启动）");
                    SetRow("t1:" + t, "跳过", -1, "上游 DNS 失败");
                    SetRow("t5:" + t, "跳过", -1, "上游 DNS 失败");
                    SetRow("t4:" + t, "跳过", -1, "上游 DNS 失败");
                    SetRow("t2:" + t, "跳过", -1, "上游 DNS 失败");
                    SetRow("t3:" + t, "跳过", -1, "上游 DNS 失败");
                    continue;
                }
                SetRow("t0:" + t, "正常", sw.ElapsedMilliseconds, string.Join(", ", resolved.ToArray()));

                // TCP
                SetStatus("检测：TCP 443 " + t + " ...");
                sw = Stopwatch.StartNew();
                if (!Net.TcpPort(t, 443, 5000))
                {
                    fail++;
                    SetRow("t1:" + t, "失败", sw.ElapsedMilliseconds, "443 端口连不上（防火墙 / 运营商 / 代理拦截）");
                    SetRow("t5:" + t, "跳过", -1, "TCP 未通");
                    SetRow("t4:" + t, "跳过", -1, "TCP 未通");
                    SetRow("t2:" + t, "跳过", -1, "TCP 未通");
                    SetRow("t3:" + t, "跳过", -1, "TCP 未通");
                    continue;
                }
                SetRow("t1:" + t, "正常", sw.ElapsedMilliseconds, "三次握手成功");

                // 边缘 IP 逐个握手：CDN 域名解析出很多 IP，个别边缘节点可能协商不了 TLS，
                // 客户端随机选中坏 IP 时就会卡住（这正是"时好时坏"的常见来源）
                SetStatus("检测：边缘 IP 逐个握手 " + t + " ...");
                List<string> allIps = new List<string>();
                try
                {
                    foreach (IPAddress a in Dns.GetHostAddresses(t))
                        if (a.AddressFamily == AddressFamily.InterNetwork && !allIps.Contains(a.ToString()))
                            allIps.Add(a.ToString());
                }
                catch { }
                int probeMax = Math.Min(allIps.Count, 8);
                int ipOk = 0;
                long ipSum = 0;
                List<string> ipBad = new List<string>();
                for (int i = 0; i < probeMax; i++)
                {
                    Net.TlsResult tr = Net.TlsHandshake(allIps[i], t, 443, 5000);
                    if (tr.Ok) { ipOk++; ipSum += tr.Ms; }
                    else if (ipBad.Count < 4) ipBad.Add(allIps[i]);
                }
                string moreTxt = allIps.Count > probeMax ? ("（共 " + allIps.Count + " 个，抽查前 " + probeMax + " 个）") : "";
                if (probeMax == 0) SetRow("t5:" + t, "跳过", -1, "没有解析到 IPv4 地址");
                else if (ipOk == probeMax) SetRow("t5:" + t, "正常", ipSum / ipOk, ipOk + "/" + probeMax + " 个边缘 IP 全部可握手，平均 " + (ipSum / ipOk) + " ms " + moreTxt);
                else if (ipOk > 0)
                {
                    warn++;
                    SetRow("t5:" + t, "警告", -1, ipOk + "/" + probeMax + " 可用；握不了手的 IP: " + string.Join(", ", ipBad.ToArray()) + "  → IDE 解析到坏 IP 时就会卡住 " + moreTxt);
                }
                else
                {
                    fail++;
                    SetRow("t5:" + t, "失败", -1, "0/" + probeMax + " 可握手；IP: " + string.Join(", ", ipBad.ToArray()) + " " + moreTxt);
                }

                // TLS 握手稳定性：连续 5 次，捕捉"时好时坏"的间歇性故障
                SetStatus("检测：TLS 握手稳定性 " + t + " (5 次) ...");
                int tlsOk = 0, tlsSum = 0, tlsMax = 0;
                string tlsErr = "";
                int tlsAvg = -1;
                for (int i = 0; i < 5; i++)
                {
                    Net.TlsResult tr = Net.TlsHandshake(t, 443, 8000);
                    if (tr.Ok)
                    {
                        tlsOk++;
                        tlsSum += (int)tr.Ms;
                        if (tr.Ms > tlsMax) tlsMax = (int)tr.Ms;
                    }
                    else tlsErr = tr.Error;
                }
                if (tlsOk > 0) tlsAvg = tlsSum / tlsOk;
                if (tlsOk == 5)
                {
                    if (tlsAvg > 400)
                    {
                        warn++;
                        SetRow("t4:" + t, "警告", tlsAvg, "5/5 成功但平均握手 " + tlsAvg + " ms（最慢 " + tlsMax + " ms）→ 握手明显偏慢，首字会很慢，体感就是一直思考中");
                    }
                    else SetRow("t4:" + t, "正常", tlsAvg, "5/5 成功，平均 " + tlsAvg + " ms，最慢 " + tlsMax + " ms");
                }
                else if (tlsOk >= 3)
                {
                    warn++;
                    SetRow("t4:" + t, "警告", -1, tlsOk + "/5 成功 → 握手时好时坏，就是“间歇性发不出去 / 一直思考中”的特征"
                        + (tlsErr.Length > 0 ? " | 失败原因: " + tlsErr : ""));
                }
                else
                {
                    fail++;
                    string hint = "";
                    if (tlsErr.IndexOf("SSPI") >= 0)
                        hint = "   ← TCP 通了却握手被拒（Schannel 层），不是网络断开，说明该链路上的边缘节点/中间设备改写了 TLS 协商";
                    SetRow("t4:" + t, "失败", -1, tlsOk + "/5 成功 → 大部分握手都失败"
                        + (tlsErr.Length > 0 ? " | 失败原因: " + tlsErr : "") + hint);
                }

                // TLS + 首字节
                SetStatus("检测：HTTPS 实测 " + t + " ...");
                Net.HttpResult r = Net.Https(t, 443, "GET", "/", null, null, 6000, 3000, 8000, 16384);
                if (string.IsNullOrEmpty(r.Protocol))
                {
                    if (tlsOk > 0)
                    {
                        warn++;
                        SetRow("t2:" + t, "警告", r.TlsMs, "本次握手失败，但稳定性测试 " + tlsOk + "/5 成功 → 属于偶发抖动，不是硬性故障: " + r.Error);
                    }
                    else
                    {
                        fail++;
                        SetRow("t2:" + t, "失败", r.TlsMs, "TLS 握手失败: " + r.Error);
                    }
                    SetRow("t3:" + t, "跳过", -1, "TLS 未通");
                    continue;
                }
                string issuer = "";
                if (!string.IsNullOrEmpty(r.Issuer))
                {
                    issuer = r.Issuer.Split(',')[0].Trim();
                    issuer = System.Text.RegularExpressions.Regex.Replace(issuer, "^(CN|O)=", "");
                }
                SetRow("t2:" + t, "正常", r.TlsMs, r.Protocol + (issuer.Length > 0 ? "  | 证书颁发者: " + issuer : "") +
                    ((r.Protocol == "Tls" || r.Protocol == "Ssl3" || r.Protocol == "Ssl2")
                        ? "   <<< 协议被降级到 TLS 1.0（不安全，且现代 CDN 会直接拒绝握手）" : ""));
                if (r.Bytes > 0)
                {
                    if (r.TtfbMs > 3000) { warn++; SetRow("t3:" + t, "警告", r.TtfbMs, "首字节 " + r.TtfbMs + " ms 偏慢，收了 " + r.Bytes + " 字节 | " + r.FirstLine); }
                    else SetRow("t3:" + t, "正常", r.TtfbMs, "收 " + r.Bytes + " 字节 | " + r.FirstLine);
                }
                else
                {
                    fail++;
                    SetRow("t3:" + t, "失败", r.Stalls, "连接建立了但收不到任何数据（读超时 " + r.Stalls + " 次）→ 数据面黑洞，这就是‘一直思考中’");
                }
            }

            // ---- 15. DeepSeek 接口实测 ----
            SetStatus("检测 15/15：DeepSeek 接口实测 ...");
            Dictionary<string, string> hdr = new Dictionary<string, string>();
            hdr["Accept"] = "text/event-stream";
            hdr["Authorization"] = "Bearer " + (string.IsNullOrEmpty(apiKey) ? "sk-invalid-probe" : apiKey);
            string body = "{\"model\":\"deepseek-chat\",\"stream\":true,\"max_tokens\":8,\"messages\":[{\"role\":\"user\",\"content\":\"hi\"}]}";
            Net.HttpResult ds = Net.Https("api.deepseek.com", 443, "POST", "/chat/completions", hdr, body, 6000, 4000, 20000, 16384);

            if (string.IsNullOrEmpty(ds.Protocol))
            {
                fail++;
                SetRow("ds1", "失败", ds.TcpMs, "TLS 握手失败: " + ds.Error);
                SetRow("ds2", "跳过", -1, "接口不通");
            }
            else if (ds.Bytes <= 0)
            {
                fail++;
                SetRow("ds1", "失败", ds.Stalls, "POST 已发出但收不到响应（读超时 " + ds.Stalls + " 次）→ 与 IDE‘一直思考中’是同一个现象");
                SetRow("ds2", "跳过", -1, "无响应数据");
            }
            else
            {
                string code = "";
                System.Text.RegularExpressions.Match m = System.Text.RegularExpressions.Regex.Match(ds.FirstLine, "\\s(\\d{3})");
                if (m.Success) code = m.Groups[1].Value;
                if (code == "401" || code == "403")
                {
                    warn++;
                    SetRow("ds1", "警告", ds.TtfbMs, "链路正常（HTTP " + code + "，未填有效 API Key 属预期）| 首字节 " + ds.TtfbMs + " ms");
                }
                else SetRow("ds1", "正常", ds.TtfbMs, "HTTP " + code + " | 首字节 " + ds.TtfbMs + " ms | 收 " + ds.Bytes + " 字节");

                if (string.IsNullOrEmpty(apiKey)) SetRow("ds2", "跳过", -1, "未填 API Key，无法实测流式首 token");
                else
                {
                    string snip = ds.Snippet;
                    System.Text.RegularExpressions.Match mc = System.Text.RegularExpressions.Regex.Match(snip, "\"content\":\"([^\"]*)\"");
                    if (mc.Success) snip = "首 token: " + mc.Groups[1].Value;
                    if (ds.TtfbMs > 5000)
                    {
                        warn++;
                        SetRow("ds2", "警告", ds.TtfbMs, "首 token " + ds.TtfbMs + " ms（偏慢，IDE 会长时间思考中）| " + snip);
                    }
                    else SetRow("ds2", "正常", ds.TtfbMs, "首 token " + ds.TtfbMs + " ms，累计 " + ds.Bytes + " 字节，读超时 " + ds.Stalls + " 次 | " + snip);
                }
            }
            return fail;
        }

        // ------------------------------------------------------------------
        //  系统配置：同样逐条平铺显示
        // ------------------------------------------------------------------
        static void AddHeader(FlowLayoutPanel flow, string text)
        {
            Panel p = new Panel();
            p.Height = 26;
            p.Margin = new Padding(0, 6, 0, 2);
            p.BackColor = Color.FromArgb(228, 236, 246);
            Label l = new Label();
            l.Dock = DockStyle.Fill;
            l.TextAlign = ContentAlignment.MiddleLeft;
            l.Padding = new Padding(8, 0, 0, 0);
            l.Text = text;
            l.Font = new Font("Microsoft YaHei UI", 9.5F, FontStyle.Bold);
            l.ForeColor = Color.FromArgb(30, 70, 130);
            p.Controls.Add(l);
            flow.Controls.Add(p);
        }

        static void AddKv(FlowLayoutPanel flow, string key, string val, Color? color)
        {
            Panel p = new Panel();
            p.Height = 24;
            p.Margin = new Padding(0, 0, 0, 1);
            p.BackColor = Color.White;
            Label v = new Label();
            v.Dock = DockStyle.Fill;
            v.TextAlign = ContentAlignment.MiddleLeft;
            v.AutoEllipsis = true;
            v.Text = val;
            if (color.HasValue) v.ForeColor = color.Value;
            Label k = new Label();
            k.Dock = DockStyle.Left;
            k.Width = 300;
            k.TextAlign = ContentAlignment.MiddleLeft;
            k.Padding = new Padding(20, 0, 0, 0);
            k.Text = key;
            k.ForeColor = Color.FromArgb(60, 60, 60);
            p.Controls.Add(v);
            p.Controls.Add(k);
            flow.Controls.Add(p);
        }

        static Color WarnColor() { return Color.FromArgb(200, 110, 0); }
        static Color FailColor() { return Color.FromArgb(200, 0, 0); }

        void BuildConfigRows()
        {
            _flowCfg.SuspendLayout();
            _flowCfg.Controls.Clear();

            // 系统
            AddHeader(_flowCfg, "系统");
            try
            {
                using (RegistryKey k = Registry.LocalMachine.OpenSubKey(@"SOFTWARE\Microsoft\Windows NT\CurrentVersion"))
                {
                    string prod = k == null ? "" : Convert.ToString(k.GetValue("ProductName"));
                    string disp = k == null ? "" : Convert.ToString(k.GetValue("DisplayVersion"));
                    string build = k == null ? "" : Convert.ToString(k.GetValue("CurrentBuild"));
                    AddKv(_flowCfg, "操作系统", prod + "  " + disp + "  (Build " + build + ")", null);
                }
            }
            catch { AddKv(_flowCfg, "操作系统", "(读取失败)", null); }
            AddKv(_flowCfg, "计算机名 / 用户", Environment.MachineName + " / " + Environment.UserName, null);
            AddKv(_flowCfg, "系统架构", Environment.Is64BitOperatingSystem ? "64 位" : "32 位", null);
            AddKv(_flowCfg, "当前是否管理员", Sys.IsAdmin() ? "是" : "否（修复操作会单独弹 UAC）", null);

            // 网络适配器
            AddHeader(_flowCfg, "网络适配器");
            NetworkInterface[] nics = NetworkInterface.GetAllNetworkInterfaces();
            foreach (NetworkInterface ni in nics)
            {
                string speed = "";
                try { if (ni.Speed > 0) speed = (ni.Speed / 1000000) + " Mbps"; } catch { }
                AddKv(_flowCfg, ni.Name,
                    ni.OperationalStatus + "   " + ni.NetworkInterfaceType + "   " + speed + "   (" + ni.Description + ")",
                    ni.OperationalStatus == OperationalStatus.Up ? (Color?)null : Color.Gray);
            }

            // IP / 网关 / DNS
            AddHeader(_flowCfg, "IP 地址 / 网关 / DNS");
            Dictionary<int, long> realMtu = Sys.GetRealMtu();
            foreach (NetworkInterface ni in nics)
            {
                List<string> ips = new List<string>();
                List<string> gws = new List<string>();
                List<string> dns = new List<string>();
                int idx = -1; long mtu = -1; bool dhcp = false;
                try
                {
                    IPInterfaceProperties ip = ni.GetIPProperties();
                    foreach (UnicastIPAddressInformation ua in ip.UnicastAddresses)
                        if (ua.Address.AddressFamily == AddressFamily.InterNetwork)
                            ips.Add(ua.Address + "/" + ua.PrefixLength);
                    foreach (GatewayIPAddressInformation g in ip.GatewayAddresses)
                        if (g.Address.AddressFamily == AddressFamily.InterNetwork) gws.Add(g.Address.ToString());
                    foreach (IPAddress a in ip.DnsAddresses)
                        if (a.AddressFamily == AddressFamily.InterNetwork) dns.Add(a.ToString());
                    try
                    {
                        IPv4InterfaceProperties v4 = ip.GetIPv4Properties();
                        idx = v4.Index; mtu = v4.Mtu; dhcp = v4.IsDhcpEnabled;
                    }
                    catch { }
                }
                catch { }
                if (ips.Count == 0 && ni.OperationalStatus != OperationalStatus.Up) continue;
                AddKv(_flowCfg, "[" + ni.Name + "] IP",
                    ips.Count > 0 ? string.Join(", ", ips.ToArray()) : "(无 IPv4)", null);
                AddKv(_flowCfg, "网关", gws.Count > 0 ? string.Join(", ", gws.ToArray()) : "(无)", null);
                AddKv(_flowCfg, "DNS", dns.Count > 0 ? string.Join(", ", dns.ToArray()) : "(无)",
                    dns.Contains("127.0.0.1") ? (Color?)WarnColor() : null);
                long showMtu = mtu;
                if (idx >= 0 && realMtu.ContainsKey(idx)) showMtu = realMtu[idx];
                string mtuTxt = (showMtu > 9216 || showMtu < 0) ? "N/A" : showMtu.ToString();
                AddKv(_flowCfg, "接口索引 / MTU / DHCP", idx + " / " + mtuTxt + " / " + (dhcp ? "是" : "否"), null);
            }

            // 系统代理
            AddHeader(_flowCfg, "系统代理 (WinINET 用户级)");
            int pe = Sys.ProxyEnable();
            string ps = Sys.ProxyValue("ProxyServer");
            string pac = Sys.ProxyValue("AutoConfigURL");
            string pov = Sys.ProxyValue("ProxyOverride");
            AddKv(_flowCfg, "ProxyEnable", pe == 1 ? "1  （代理已启用，IDE 会跟随）" : pe.ToString(), pe == 1 ? (Color?)WarnColor() : null);
            AddKv(_flowCfg, "ProxyServer", string.IsNullOrEmpty(ps) ? "(空)" : ps, null);
            AddKv(_flowCfg, "AutoConfigURL", string.IsNullOrEmpty(pac) ? "(空)" : pac, null);
            AddKv(_flowCfg, "ProxyOverride", string.IsNullOrEmpty(pov) ? "(空)" : pov, null);
            AddKv(_flowCfg, "本地代理端口监听",
                LocalProxyListenText(), null);

            // WinHTTP
            AddHeader(_flowCfg, "WinHTTP 代理");
            string wh = Sys.RunCmd("netsh winhttp show proxy");
            foreach (string line in wh.Split('\n'))
            {
                string t = line.Trim();
                if (t.Length > 0) AddKv(_flowCfg, "", t, null);
            }

            // HOSTS
            AddHeader(_flowCfg, "HOSTS 文件  (" + Sys.HostsPath() + ")");
            List<string> hs = Sys.HostsActiveLines();
            AddKv(_flowCfg, "生效条目数", hs.Count.ToString(), null);
            if (hs.Count == 0) AddKv(_flowCfg, "", "(无自定义条目，正常)", Color.Gray);
            foreach (string l in hs)
            {
                string low = l.ToLower();
                bool bad = low.IndexOf("trae") >= 0 || low.IndexOf("deepseek") >= 0;
                AddKv(_flowCfg, "", l + (bad ? "     << 与 Trae/DeepSeek 相关，建议清理" : ""), bad ? (Color?)FailColor() : null);
            }

            // Winsock
            AddHeader(_flowCfg, "Winsock 目录 (LSP)");
            int lsp = Sys.WinsockLspCount();
            AddKv(_flowCfg, "Catalog_Entries 条目数", lsp < 0 ? "(读取失败)" : lsp.ToString(),
                lsp > 20 ? (Color?)WarnColor() : null);
            AddKv(_flowCfg, "说明", "原生 Windows 一般 10~14 条，明显偏多说明被安全/加速软件注入了 LSP", Color.Gray);

            // TLS 策略
            AddHeader(_flowCfg, "TLS 协议策略");
            AddKv(_flowCfg, "当前使用档位", Net.ProtocolName(), null);
            AddKv(_flowCfg, "说明", "正常应为 TLS 1.2 + 1.3；若显示已降级为纯 TLS 1.2，说明本机 Schannel 不支持 TLS 1.3", Color.Gray);

            // TCP 统计
            AddHeader(_flowCfg, "TCP 统计");
            try
            {
                TcpStatistics ts = IPGlobalProperties.GetIPGlobalProperties().GetTcpIPv4Statistics();
                double retPct = ts.SegmentsSent > 0 ? (100.0 * ts.SegmentsResent / ts.SegmentsSent) : 0;
                double failPct = ts.ConnectionsInitiated > 0 ? (100.0 * ts.FailedConnectionAttempts / ts.ConnectionsInitiated) : 0;
                AddKv(_flowCfg, "当前连接数", ts.CurrentConnections.ToString(), null);
                AddKv(_flowCfg, "累计发起连接", ts.ConnectionsInitiated.ToString(), null);
                AddKv(_flowCfg, "失败连接尝试", ts.FailedConnectionAttempts + "   占 " + failPct.ToString("F1") + "%   （含本工具探测已关闭端口产生的失败，不代表外网失败率）", null);
                AddKv(_flowCfg, "段 发送 / 接收", ts.SegmentsSent + " / " + ts.SegmentsReceived, null);
                AddKv(_flowCfg, "段 重传", ts.SegmentsResent + "   占 " + retPct.ToString("F1") + "%",
                    retPct > 2 ? (Color?)WarnColor() : null);
                AddKv(_flowCfg, "连接重置(reset) 发出", ts.ResetsSent.ToString(), null);
                AddKv(_flowCfg, "判读", "重点看重传率：>2% 说明链路层质量差（Wi-Fi 干扰 / 网线 / 路由器），会直接导致长连接卡顿", Color.Gray);
            }
            catch (Exception ex) { AddKv(_flowCfg, "读取失败", ex.Message, null); }

            // 连接配置
            AddHeader(_flowCfg, "网络配置文件");
            try
            {
                foreach (NetworkInterface ni in nics)
                {
                    if (ni.OperationalStatus != OperationalStatus.Up) continue;
                    if (ni.NetworkInterfaceType == NetworkInterfaceType.Loopback) continue;
                    AddKv(_flowCfg, ni.Name, ni.NetworkInterfaceType + "   " + ni.OperationalStatus, null);
                }
            }
            catch { }

            _flowCfg.ResumeLayout();
            ResizeRows(_flowCfg);
            SetStatus("配置已刷新：" + DateTime.Now.ToString("HH:mm:ss"));
        }

        string LocalProxyListenText()
        {
            int[] cand = new int[] { 7890, 7897, 7891, 1080, 10808, 10809, 8889, 2080 };
            List<string> up = new List<string>();
            foreach (int p in cand) if (Net.TcpPort("127.0.0.1", p, 300)) up.Add(p.ToString());
            return up.Count > 0 ? string.Join(", ", up.ToArray()) + "   正在监听" : "无";
        }

        void FillInterfaces()
        {
            _cmbIf.Items.Clear();
            foreach (NetworkInterface ni in NetworkInterface.GetAllNetworkInterfaces())
            {
                if (ni.OperationalStatus != OperationalStatus.Up) continue;
                if (ni.NetworkInterfaceType == NetworkInterfaceType.Loopback) continue;
                int idx = -1;
                try { idx = ni.GetIPProperties().GetIPv4Properties().Index; } catch { }
                if (idx < 0) continue;
                _cmbIf.Items.Add(new IfItem(idx, ni.Name + "  (索引 " + idx + ")"));
            }
            if (_cmbIf.Items.Count > 0) _cmbIf.SelectedIndex = 0;
        }

        class IfItem
        {
            public int Index; public string Label;
            public IfItem(int i, string l) { Index = i; Label = l; }
            public override string ToString() { return Label; }
        }

        // ------------------------------------------------------------------
        //  接口监控：轮询本机 TCP 连接表，跟踪被监控目标的连接生命周期
        // ------------------------------------------------------------------
        void ToggleMonitor()
        {
            if (_monRunning) StopMonitor(); else StartMonitor();
        }

        void StartMonitor()
        {
            if (_monTimer == null)
            {
                _monTimer = new System.Windows.Forms.Timer();
                _monTimer.Interval = 1000;               // 每秒采样一次连接表
                _monTimer.Tick += delegate { MonTick(); };
            }
            _monRunning = true;
            _monAlive.Clear();
            _monDnsTime = DateTime.MinValue;
            RefreshMonIps();
            _monTimer.Start();
            _btnMon.Text = "停止监控";
            AddMonEvent("系统", "监控已启动", "-", "-", "-", "-",
                "每秒采样一次本机 TCP 连接表，监控目标：" + string.Join(" / ", MonHosts));
        }

        void StopMonitor()
        {
            _monRunning = false;
            if (_monTimer != null) _monTimer.Stop();
            _btnMon.Text = "开始监控";
            AddMonEvent("系统", "监控已停止", "-", "-", "-", "-", "");
        }

        // 后台线程解析被监控域名的 IP（CDN 会轮换，每 30 秒刷新一次）
        void RefreshMonIps()
        {
            if (_monDnsBusy) return;
            _monDnsBusy = true;
            Thread th = new Thread(delegate ()
            {
                try
                {
                    Dictionary<string, string> map = new Dictionary<string, string>();
                    foreach (string h in MonHosts)
                    {
                        try
                        {
                            foreach (IPAddress a in Dns.GetHostAddresses(h))
                                if (a.AddressFamily == AddressFamily.InterNetwork) map[a.ToString()] = h;
                        }
                        catch { }
                    }
                    if (map.Count > 0) _monIpHits = map;
                }
                catch { }
                _monDnsBusy = false;
            });
            th.IsBackground = true;
            th.Start();
        }

        void MonTick()
        {
            if (!_monRunning) return;
            int active = 0;
            try
            {
                if ((DateTime.Now - _monDnsTime).TotalSeconds > 30)
                {
                    _monDnsTime = DateTime.Now;
                    RefreshMonIps();
                }

                Dictionary<string, string> ips = _monIpHits;
                bool self = _running;      // 检测正在跑 -> 这段时间的新连接多半是本工具自己的
                HashSet<string> seen = new HashSet<string>();

                foreach (TcpConnectionInformation ci in IPGlobalProperties.GetIPGlobalProperties().GetActiveTcpConnections())
                {
                    if (ci.RemoteEndPoint.Port != 443) continue;
                    string ip = ci.RemoteEndPoint.Address.ToString();
                    string host;
                    if (!ips.TryGetValue(ip, out host)) continue;

                    string key = ci.LocalEndPoint + ">" + ci.RemoteEndPoint;
                    seen.Add(key);
                    if (ci.State != TcpState.TimeWait) active++;

                    MonConn mc;
                    if (!_monAlive.TryGetValue(key, out mc))
                    {
                        mc = new MonConn();
                        mc.Key = key;
                        mc.Host = host;
                        mc.RemoteIp = ip;
                        mc.LocalPort = ci.LocalEndPoint.Port;
                        mc.LastState = ci.State;
                        mc.Self = self;
                        _monAlive[key] = mc;
                        AddMonEvent("新建", host, ip, mc.LocalPort.ToString(), ci.State.ToString(), "-",
                            (self ? "[本工具检测] " : "") + DescribeNew(ci.State));
                    }
                    else if (mc.LastState != ci.State)
                    {
                        string note = DescribeTransition(mc.LastState, ci.State);
                        mc.LastState = ci.State;
                        AddMonEvent("状态", mc.Host, mc.RemoteIp, mc.LocalPort.ToString(), "→ " + ci.State,
                            FormatDur(DateTime.Now - mc.FirstSeen), (mc.Self ? "[本工具检测] " : "") + note);
                    }
                    else if (ci.State == TcpState.SynSent && !mc.SynAlarmed &&
                             (DateTime.Now - mc.FirstSeen).TotalSeconds >= MonSynAlarmSec)
                    {
                        mc.SynAlarmed = true;
                        AddMonEvent("异常", mc.Host, mc.RemoteIp, mc.LocalPort.ToString(), "SynSent",
                            FormatDur(DateTime.Now - mc.FirstSeen),
                            "SYN 已发出但 " + MonSynAlarmSec + " 秒无响应 → 请求发出去了、对端没回");
                    }
                }

                // 上一轮还在、这一轮消失的，判定为已结束
                List<string> gone = new List<string>();
                foreach (KeyValuePair<string, MonConn> kv in _monAlive)
                    if (!seen.Contains(kv.Key)) gone.Add(kv.Key);
                foreach (string k in gone)
                {
                    MonConn mc = _monAlive[k];
                    double dur = (DateTime.Now - mc.FirstSeen).TotalSeconds;
                    string kind = "结束";
                    string note;
                    if (mc.LastState == TcpState.SynSent)
                    {
                        kind = "异常";
                        note = "SYN 全程无响应，连接从未建立 → 请求发出去了但没能连上对端";
                    }
                    else if (dur < 2)
                    {
                        kind = "异常";
                        note = "建立后 " + dur.ToString("F1") + " 秒即断开，疑似被 RST 重置（安全软件 / 中间设备 / 对端拒绝）";
                    }
                    else if (mc.LastState == TcpState.TimeWait) note = "连接正常关闭";
                    else note = "连接已结束（最后状态 " + mc.LastState + "）";
                    if (mc.Self) note = "[本工具检测] " + note;
                    AddMonEvent(kind, mc.Host, mc.RemoteIp, mc.LocalPort.ToString(), "× " + mc.LastState,
                        FormatDur(DateTime.Now - mc.FirstSeen), note);
                    _monAlive.Remove(k);
                }
            }
            catch { }
            UpdateMonStatus(active);
        }

        void UpdateMonStatus(int active)
        {
            if (_lblMonStatus == null) return;
            _lblMonStatus.Text = string.Format("监控中 · 目标 {0} 个域名（已解析 {1} 个 IP） · 当前活跃连接 {2} · 累计事件 {3}",
                MonHosts.Length, _monIpHits.Count, active, _monEventCount);
        }

        void AddMonEvent(string kind, string target, string ip, string localPort, string state, string dur, string note)
        {
            if (_lstMon == null) return;
            _monEventCount++;
            ListViewItem it = new ListViewItem(DateTime.Now.ToString("HH:mm:ss.fff"));
            it.SubItems.Add(kind);
            it.SubItems.Add(target);
            it.SubItems.Add(ip);
            it.SubItems.Add(localPort);
            it.SubItems.Add(state);
            it.SubItems.Add(dur);
            it.SubItems.Add(note);
            switch (kind)
            {
                case "异常": it.ForeColor = Color.FromArgb(200, 0, 0); break;
                case "新建": it.ForeColor = Color.FromArgb(0, 70, 160); break;
                case "结束": it.ForeColor = Color.FromArgb(130, 130, 130); break;
                case "系统": it.ForeColor = Color.FromArgb(0, 130, 0); break;
                default: it.ForeColor = Color.FromArgb(50, 50, 50); break;
            }
            _lstMon.Items.Add(it);
            if (_lstMon.Items.Count > MonMaxRows)
            {
                int drop = _lstMon.Items.Count - MonMaxRows + 500;
                for (int i = 0; i < drop && _lstMon.Items.Count > 0; i++) _lstMon.Items.RemoveAt(0);
            }
            if (_chkMonScroll != null && _chkMonScroll.Checked && _lstMon.Items.Count > 0)
                _lstMon.EnsureVisible(_lstMon.Items.Count - 1);
        }

        static string DescribeNew(TcpState s)
        {
            switch (s)
            {
                case TcpState.SynSent: return "已发出 SYN，正在等待对端回应";
                case TcpState.Established: return "连接已建立（握手在 1 秒采样间隔内完成）";
                case TcpState.SynReceived: return "收到对端 SYN";
                case TcpState.FinWait1:
                case TcpState.FinWait2: return "本端正在关闭连接";
                case TcpState.CloseWait: return "对端已关闭，本端待关闭";
                case TcpState.TimeWait: return "连接已关闭（TIME_WAIT，属正常残留）";
                case TcpState.Closing:
                case TcpState.LastAck: return "连接关闭中";
                default: return "";
            }
        }

        static string DescribeTransition(TcpState from, TcpState to)
        {
            if (from == TcpState.SynSent && to == TcpState.Established) return "TCP 握手成功，连接建立";
            if (from == TcpState.SynSent && (to == TcpState.Closed || to == TcpState.TimeWait)) return "握手失败（SYN 被拒绝或超时）";
            if (to == TcpState.Established) return "连接建立";
            if (to == TcpState.CloseWait) return "对端已关闭发送通道（服务端主动结束）";
            if (to == TcpState.TimeWait) return "本端主动关闭，进入 TIME_WAIT（正常）";
            if (to == TcpState.FinWait1 || to == TcpState.FinWait2) return "本端正在关闭连接";
            if (to == TcpState.LastAck) return "双方关闭中";
            return from + " → " + to;
        }

        static string FormatDur(TimeSpan t)
        {
            if (t.TotalSeconds < 60) return t.TotalSeconds.ToString("F1") + " s";
            return ((int)t.TotalMinutes) + " 分 " + t.Seconds + " 秒";
        }

        // ------------------------------------------------------------------
        void DoRepair()
        {
            List<string> tasks = new List<string>();
            List<string> descs = new List<string>();
            foreach (object o in _chkFix.CheckedItems)
            {
                string s = o.ToString();
                int bar = s.IndexOf('|');
                tasks.Add(s.Substring(0, bar));
                descs.Add(s.Substring(bar + 1));
            }
            if (_chkMtu.Checked)
            {
                if (_cmbIf.SelectedIndex < 0)
                {
                    MessageBox.Show("请先选择要调整 MTU 的网卡。", "提示", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                    return;
                }
                IfItem it = (IfItem)_cmbIf.SelectedItem;
                tasks.Add("mtu:" + it.Index + ":" + (int)_numMtu.Value);
                descs.Add("调整网卡 MTU -> " + (int)_numMtu.Value + "   " + it.Label);
            }
            if (tasks.Count == 0)
            {
                MessageBox.Show("请至少勾选一项修复动作。", "提示", MessageBoxButtons.OK, MessageBoxIcon.Warning);
                return;
            }

            string msg = "即将执行以下修复（需要管理员权限）：\r\n\r\n  " + string.Join("\r\n  ", descs.ToArray()) + "\r\n\r\n确认继续？";
            if (MessageBox.Show(msg, "确认修复", MessageBoxButtons.YesNo, MessageBoxIcon.Question) != DialogResult.Yes) return;

            _txtFixLog.Text = "正在请求管理员权限...\r\n";
            string logPath = Path.Combine(Path.GetTempPath(), "trae_net_doctor_repair_" + DateTime.Now.ToString("yyyyMMdd_HHmmss") + ".log");
            string args = "--repair " + string.Join(",", tasks.ToArray()) + " --log \"" + logPath + "\"";

            try
            {
                ProcessStartInfo psi = new ProcessStartInfo(Application.ExecutablePath, args);
                psi.UseShellExecute = true;
                psi.Verb = "runas";
                psi.WindowStyle = ProcessWindowStyle.Hidden;
                using (Process p = Process.Start(psi))
                {
                    p.WaitForExit();
                }
            }
            catch (Exception ex)
            {
                _txtFixLog.Text = "已取消（未获得管理员权限）\r\n" + ex.Message;
                SetStatus("修复已取消。");
                return;
            }

            Thread.Sleep(300);
            if (File.Exists(logPath))
            {
                try { _txtFixLog.Text = File.ReadAllText(logPath, Encoding.UTF8); }
                catch (Exception ex) { _txtFixLog.Text = "读取日志失败: " + ex.Message; }
            }
            else _txtFixLog.Text = "未拿到修复日志（子进程可能启动失败）。";

            if (tasks.Contains("winsock") || tasks.Contains("tcpip"))
                MessageBox.Show("修复已完成。\r\n\r\n你执行了 Winsock / TCP-IP 重置，必须重启电脑才会生效。",
                    "需要重启", MessageBoxButtons.OK, MessageBoxIcon.Information);

            BuildConfigRows();
            SetStatus("修复完成。");
        }

        string BuildReportText()
        {
            StringBuilder sb = new StringBuilder();
            sb.AppendLine("Trae / DeepSeek 通路诊断报告   " + DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss"));
            sb.AppendLine(new string('=', 88));
            sb.AppendLine();
            sb.AppendLine("【通路检测结果】");
            foreach (ItemRow r in _checkRows)
            {
                string st = r.Status.Text;
                if (st == "待检测") continue;
                sb.AppendLine("  " + st + "\t" + r.Name.Text + "\t" + r.Ms.Text + "\t" + r.Detail.Text);
            }
            sb.AppendLine();
            sb.AppendLine("【系统配置】");
            foreach (Control c in _flowCfg.Controls)
            {
                if (!(c is Panel)) continue;
                if (c.Controls.Count >= 2)
                {
                    string k = c.Controls[1].Text;
                    string v = c.Controls[0].Text;
                    if (k.Length == 0) sb.AppendLine("    " + v);
                    else sb.AppendLine("  " + k.PadRight(24) + v);
                }
                else if (c.Controls.Count == 1) sb.AppendLine("  == " + c.Controls[0].Text + " ==");
            }
            sb.AppendLine();
            sb.AppendLine("【修复日志】");
            sb.AppendLine(_txtFixLog.Text);
            return sb.ToString();
        }

        void CopyReport()
        {
            try
            {
                Clipboard.SetText(BuildReportText());
                SetStatus("报告已复制到剪贴板。");
                MessageBox.Show("报告已复制到剪贴板，可直接粘贴给我分析。", "完成", MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
            catch (Exception ex) { SetStatus("复制失败: " + ex.Message); }
        }

        void ExportReport()
        {
            try
            {
                SaveFileDialog dlg = new SaveFileDialog();
                dlg.Filter = "文本文件 (*.txt)|*.txt";
                dlg.FileName = "trae_net_report_" + DateTime.Now.ToString("yyyyMMdd_HHmmss") + ".txt";
                if (dlg.ShowDialog() == DialogResult.OK)
                {
                    File.WriteAllText(dlg.FileName, BuildReportText(), new UTF8Encoding(true));
                    SetStatus("报告已导出: " + dlg.FileName);
                }
            }
            catch (Exception ex) { SetStatus("导出失败: " + ex.Message); }
        }
    }
}
