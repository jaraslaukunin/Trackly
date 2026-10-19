// supabase/functions/emergency-email/index.ts
//
// Экстренное уведомление. Отправляет красивое письмо через Resend
// с именем пациента и временем отправки.
//
// Принимает: { to: string, patientName?: string, text?: string, html?: string, subject?: string }
// Возвращает: данные от Resend API

const RESEND_API_KEY = Deno.env.get("RESEND_API_KEY")!;

type EmergencyPayload = {
  to: string;
  patientName?: string;
  subject?: string;
  text?: string;
  html?: string;
};

Deno.serve(async (req: Request): Promise<Response> => {
  if (req.method !== "POST") {
    return new Response("Method not allowed", { status: 405 });
  }

  if (!RESEND_API_KEY) {
    return new Response(
      JSON.stringify({ error: "RESEND_API_KEY not configured" }),
      { status: 500, headers: { "Content-Type": "application/json" } }
    );
  }

  let body: EmergencyPayload;
  try {
    body = await req.json();
  } catch {
    return new Response(
      JSON.stringify({ error: "Invalid JSON body" }),
      { status: 400, headers: { "Content-Type": "application/json" } }
    );
  }

  const to = body.to?.trim();
  if (!to) {
    return new Response(
      JSON.stringify({ error: 'Missing "to" field' }),
      { status: 400, headers: { "Content-Type": "application/json" } }
    );
  }

  const patientName = (body.patientName ?? "Пользователь Trackly").trim();
  const now = new Date();
  const dateStr = now.toLocaleDateString("ru-RU", {
    day: "numeric",
    month: "long",
    year: "numeric",
  });
  const timeStr = now.toLocaleTimeString("ru-RU", {
    hour: "2-digit",
    minute: "2-digit",
  });

  const subject = (body.subject ?? `⚠️ Экстренное уведомление: ${patientName}`).trim();

  const plainText =
    body.text ??
    `${patientName} отправил(а) экстренное уведомление из Trackly.\n\n` +
      `Время отправки: ${dateStr} в ${timeStr}\n\n` +
      "Попробуйте связаться с ним как можно скорее.";

  const html =
    body.html ??
    `<!DOCTYPE html>
<html>
<head><meta charset="utf-8"></head>
<body style="margin:0;padding:0;background:#FEF2F2;font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;">
<table width="100%" cellpadding="0" cellspacing="0" style="background:#FEF2F2;padding:40px 0;">
<tr><td align="center">
<table width="440" cellpadding="0" cellspacing="0" style="background:#FFFFFF;border-radius:16px;overflow:hidden;box-shadow:0 2px 16px rgba(220,38,38,0.08);">

  <!-- Emergency header -->
  <tr>
    <td style="background:linear-gradient(135deg,#DC2626,#B91C1C);padding:28px 32px;text-align:center;">
      <div style="font-size:14px;font-weight:600;color:rgba(255,255,255,0.85);letter-spacing:1px;text-transform:uppercase;">⚠️&nbsp; Экстренное уведомление</div>
    </td>
  </tr>

  <!-- Body -->
  <tr>
    <td style="padding:32px;">
      <p style="margin:0 0 4px;font-size:13px;color:#94A3B8;letter-spacing:0.3px;">ПАЦИЕНТ</p>
      <p style="margin:0 0 24px;font-size:22px;font-weight:800;color:#1E1B2E;">${patientName}</p>

      <p style="margin:0 0 20px;font-size:15px;color:#3D3555;line-height:1.7;">
        Отправил(а) экстренное уведомление из приложения <strong>Trackly</strong>.
        Попробуйте связаться с ним как можно скорее.
      </p>

      <!-- Time badge -->
      <table width="100%" cellpadding="0" cellspacing="0" style="margin-bottom:24px;">
        <tr>
          <td>
            <table cellpadding="0" cellspacing="0" style="background:#FFF7ED;border:1px solid #FED7AA;border-radius:10px;">
              <tr>
                <td style="padding:10px 16px;">
                  <span style="font-size:12px;color:#C2410C;">Отправлено: ${dateStr} в ${timeStr}</span>
                </td>
              </tr>
            </table>
          </td>
        </tr>
      </table>

      <!-- Warning note -->
      <table width="100%" cellpadding="0" cellspacing="0" style="background:#FEF2F2;border-left:3px solid #DC2626;border-radius:0 8px 8px 0;">
        <tr>
          <td style="padding:12px 16px;">
            <span style="font-size:13px;color:#991B1B;line-height:1.6;">
              Если вы не можете связаться с пациентом и есть основания полагать, что ему требуется медицинская помощь — <strong>позвоните 103 или 112</strong>.
            </span>
          </td>
        </tr>
      </table>
    </td>
  </tr>

  <!-- Footer -->
  <tr>
    <td style="border-top:1px solid #FEE2E2;padding:16px 32px;text-align:center;">
      <span style="font-size:11px;color:#B8AECF;">Trackly — персональный контроль эпилепсии и терапии</span>
    </td>
  </tr>
</table>
</td></tr>
</table>
</body>
</html>`;

  const resendRes = await fetch("https://api.resend.com/emails", {
    method: "POST",
    headers: {
      "Content-Type": "application/json",
      Authorization: `Bearer ${RESEND_API_KEY}`,
    },
    body: JSON.stringify({
      from: "Trackly Alerts <alerts@mail.tracklyapp.xyz>",
      to: [to],
      subject,
      text: plainText,
      html,
    }),
  });

  const data = await resendRes.json();

  return new Response(JSON.stringify(data), {
    status: resendRes.ok ? 200 : resendRes.status,
    headers: { "Content-Type": "application/json" },
  });
});
