use std::fmt::{Display, Formatter};

#[derive(Clone, Copy, Debug, Eq, PartialEq)]
pub enum ExitKind {
    Evidence,
    Usage,
    Timeout,
}

impl ExitKind {
    #[must_use]
    pub const fn code(self) -> i32 {
        match self {
            Self::Evidence => 1,
            Self::Usage => 2,
            Self::Timeout => 3,
        }
    }
}

#[derive(Debug)]
pub struct AppError {
    pub kind: ExitKind,
    pub message: String,
}

impl AppError {
    #[must_use]
    pub fn evidence(message: impl Into<String>) -> Self {
        Self {
            kind: ExitKind::Evidence,
            message: message.into(),
        }
    }

    #[must_use]
    pub fn usage(message: impl Into<String>) -> Self {
        Self {
            kind: ExitKind::Usage,
            message: message.into(),
        }
    }

    #[must_use]
    pub fn timeout(message: impl Into<String>) -> Self {
        Self {
            kind: ExitKind::Timeout,
            message: message.into(),
        }
    }

    #[must_use]
    pub fn context(self, context: impl Display) -> Self {
        Self {
            kind: self.kind,
            message: format!("{context}: {}", self.message),
        }
    }
}

impl Display for AppError {
    fn fmt(&self, formatter: &mut Formatter<'_>) -> std::fmt::Result {
        formatter.write_str(&self.message)
    }
}

impl std::error::Error for AppError {}

pub type AppResult<T> = Result<T, AppError>;

pub trait UsageContext<T> {
    fn usage_context(self, context: impl Display) -> AppResult<T>;
}

impl<T, E: Display> UsageContext<T> for Result<T, E> {
    fn usage_context(self, context: impl Display) -> AppResult<T> {
        self.map_err(|error| AppError::usage(format!("{context}: {error}")))
    }
}

pub trait EvidenceContext<T> {
    fn evidence_context(self, context: impl Display) -> AppResult<T>;
}

impl<T, E: Display> EvidenceContext<T> for Result<T, E> {
    fn evidence_context(self, context: impl Display) -> AppResult<T> {
        self.map_err(|error| AppError::evidence(format!("{context}: {error}")))
    }
}
