// supabase/functions/send-verification-code/index.ts
//
// Вызывается после signUp. Генерирует 6-значный код, сохраняет в БД,
// отправляет на почту через Resend.
//
// Принимает: { email: string, name?: string }
// Возвращает: { success: true } или ошибку

import { createClient } from "npm:@supabase/supabase-js@2";

const RESEND_API_KEY = Deno.env.get("RESEND_API_KEY")!;
const SUPABASE_URL = Deno.env.get("SUPABASE_URL")!;
const SUPABASE_SERVICE_ROLE_KEY = Deno.env.get("SUPABASE_SERVICE_ROLE_KEY")!;

const supabase = createClient(SUPABASE_URL, SUPABASE_SERVICE_ROLE_KEY);

function generateCode(): string {
  return String(Math.floor(100000 + Math.random() * 900000));
}

Deno.serve(async (req: Request): Promise<Response> => {
  if (req.method !== "POST") {
    return new Response("Method not allowed", { status: 405 });
  }

  let email: string;
  let name: string;

  try {
    const body = await req.json();
    email = (body.email ?? "").trim().toLowerCase();
    name = (body.name ?? "").trim();
  } catch {
    return new Response(
      JSON.stringify({ error: "Invalid JSON" }),
      { status: 400, headers: { "Content-Type": "application/json" } }
    );
  }

  if (!email) {
    return new Response(
      JSON.stringify({ error: "Missing email" }),
      { status: 400, headers: { "Content-Type": "application/json" } }
    );
  }

  const code = generateCode();
  const displayName = name || email.split("@")[0];

  // Сохраняем код в БД (service_role client пишет в public)
  const { error: dbError } = await supabase
    .from("verification_codes")
    .insert({
      email,
      code,
      expires_at: new Date(Date.now() + 10 * 60 * 1000).toISOString(),
    });

  if (dbError) {
    console.error("DB insert error:", dbError);
    return new Response(
      JSON.stringify({ error: "Failed to save code" }),
      { status: 500, headers: { "Content-Type": "application/json" } }
    );
  }

  // Отправляем через Resend
  const html = `
<!DOCTYPE html>
<html>
<head><meta charset="utf-8"></head>
<body style="margin:0;padding:0;background:#F7F5FF;font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',sans-serif;">
<table width="100%" cellpadding="0" cellspacing="0" style="background:#F7F5FF;padding:40px 0;">
<tr><td align="center">
<table width="440" cellpadding="0" cellspacing="0" style="background:#FFFFFF;border-radius:16px;overflow:hidden;box-shadow:0 2px 16px rgba(124,92,250,0.10);">
  <!-- Header -->
  <tr>
    <td style="background:linear-gradient(135deg,#7C5CFA,#5C3DD6);padding:28px 32px;text-align:center;">
      <div style="font-size:16px;font-weight:700;color:#FFFFFF;letter-spacing:0.5px;">Trackly</div>
    </td>
  </tr>
  <!-- Body -->
  <tr>
    <td style="padding:32px;">
      <p style="margin:0 0 8px;font-size:20px;font-weight:700;color:#231A34;">Привет, ${displayName}!</p>
      <p style="margin:0 0 24px;font-size:14px;color:#5D5379;line-height:1.6;">
        Твой код подтверждения для регистрации в Trackly:
      </p>

      <!-- Code block -->
      <table width="100%" cellpadding="0" cellspacing="0">
        <tr>
          <td align="center" style="padding:20px 0 24px;">
            <div style="display:inline-block;background:#F3EEFF;border-radius:14px;padding:18px 36px;">
              <span style="font-size:32px;font-weight:800;color:#7C5CFA;letter-spacing:6px;font-family:'JetBrains Mono',monospace;">${code}</span>
            </div>
          </td>
        </tr>
      </table>

      <p style="margin:0 0 8px;font-size:12px;color:#94A3B8;line-height:1.6;">
        Код действует 10 минут. Если вы не регистрировались в Trackly — просто проигнорируйте это письмо.
      </p>
    </td>
  </tr>
  <!-- Footer -->
  <tr>
    <td style="border-top:1px solid #F0ECFF;padding:16px 32px;text-align:center;">
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
      from: "Trackly <noreply@mail.tracklyapp.xyz>",
      to: [email],
      subject: `Код подтверждения: ${code}`,
      text: `Твой код подтверждения для Trackly: ${code}. Код действует 10 минут.`,
      html,
    }),
  });

  if (!resendRes.ok) {
    const errData = await resendRes.json();
    console.error("Resend error:", errData);
    return new Response(
      JSON.stringify({ error: "Failed to send email" }),
      { status: 500, headers: { "Content-Type": "application/json" } }
    );
  }

  return new Response(
    JSON.stringify({ success: true }),
    { status: 200, headers: { "Content-Type": "application/json" } }
  );
});
