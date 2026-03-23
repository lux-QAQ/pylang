    // --- Reraise ---
    m_builder.SetInsertPoint(reraise_bb);
    // ✅ 从 alloca 获取类型，不依赖外部 lp_ty 变量
    auto *lp_ty = try_ctx.lp_alloca->getAllocatedType();
    auto *saved_lp = m_builder.CreateLoad(lp_ty, try_ctx.lp_alloca, "saved.lp");
    m_builder.CreateResume(saved_lp);