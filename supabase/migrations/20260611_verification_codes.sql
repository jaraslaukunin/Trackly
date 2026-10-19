-- Таблица для 6-значных кодов верификации
CREATE TABLE IF NOT EXISTS public.verification_codes (
    id          BIGSERIAL PRIMARY KEY,
    email       TEXT        NOT NULL,
    code        TEXT        NOT NULL,
    expires_at  TIMESTAMPTZ NOT NULL DEFAULT (now() + INTERVAL '10 minutes'),
    created_at  TIMESTAMPTZ NOT NULL DEFAULT now(),
    used        BOOLEAN     NOT NULL DEFAULT false
);

-- Индекс для быстрого поиска по email
CREATE INDEX IF NOT EXISTS idx_verification_codes_email
    ON public.verification_codes (email, created_at DESC);

-- Автоочистка просроченных кодов (раз в час)
SELECT cron.schedule(
    'cleanup-verification-codes',
    '0 * * * *',
    $$ DELETE FROM public.verification_codes WHERE expires_at < now() OR used = true $$
);
