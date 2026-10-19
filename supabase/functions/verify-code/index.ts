// supabase/functions/verify-code/index.ts
//
// Проверяет 6-значный код из БД. При успехе помечает код использованным
// и возвращает success.
//
// Принимает: { email: string, code: string }
// Возвращает: { success: true } или ошибку

import { createClient } from "npm:@supabase/supabase-js@2";

const SUPABASE_URL = Deno.env.get("SUPABASE_URL")!;
const SUPABASE_SERVICE_ROLE_KEY = Deno.env.get("SUPABASE_SERVICE_ROLE_KEY")!;

const supabase = createClient(SUPABASE_URL, SUPABASE_SERVICE_ROLE_KEY);

Deno.serve(async (req: Request): Promise<Response> => {
  if (req.method !== "POST") {
    return new Response("Method not allowed", { status: 405 });
  }

  let email: string;
  let code: string;

  try {
    const body = await req.json();
    email = (body.email ?? "").trim().toLowerCase();
    code = (body.code ?? "").trim();
  } catch {
    return new Response(
      JSON.stringify({ error: "Invalid JSON" }),
      { status: 400, headers: { "Content-Type": "application/json" } }
    );
  }

  if (!email || !code) {
    return new Response(
      JSON.stringify({ error: "Missing email or code" }),
      { status: 400, headers: { "Content-Type": "application/json" } }
    );
  }

  // Ищем последний неиспользованный непросроченный код
  const { data, error } = await supabase
    .from("verification_codes")
    .select("*")
    .eq("email", email)
    .eq("used", false)
    .gt("expires_at", new Date().toISOString())
    .order("created_at", { ascending: false })
    .limit(1);

  if (error) {
    console.error("DB query error:", error);
    return new Response(
      JSON.stringify({ error: "Database error" }),
      { status: 500, headers: { "Content-Type": "application/json" } }
    );
  }

  if (!data || data.length === 0) {
    return new Response(
      JSON.stringify({ error: "Код не найден или истёк" }),
      { status: 400, headers: { "Content-Type": "application/json" } }
    );
  }

  const record = data[0];

  if (record.code !== code) {
    return new Response(
      JSON.stringify({ error: "Неверный код" }),
      { status: 400, headers: { "Content-Type": "application/json" } }
    );
  }

  // Помечаем использованным
  await supabase
    .from("verification_codes")
    .update({ used: true })
    .eq("id", record.id);

  return new Response(
    JSON.stringify({ success: true }),
    { status: 200, headers: { "Content-Type": "application/json" } }
  );
});
