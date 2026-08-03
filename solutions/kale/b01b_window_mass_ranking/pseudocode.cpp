/*
KALE B01B — FOREGROUND-WINDOW-MASS STRATEGIC RANKING

CONTROL
    KALE B01A (Push 21B plus the proven Push 19A T2 terminal flank).

HYPOTHESIS
    The current strategic loss averages candidate damage equally across every
    affected view. The official evaluator averages 11x11 windows whose center
    is foreground in either rendering. Weighting each affected view by its
    foreground-window mass should prefer a collapse whose damage occupies less
    of the evaluated object, exposing a safer T3 frontier without changing the
    topology or number of strategic probes.

TOP LEVEL
    execute every B01A branch, schedule, hard guard, and T2 terminal flank
    unchanged

    whenever a strategic endpoint-weld candidate is evaluated (T2 or T3):
        affectedFaces = incident faces of kept endpoint union removed endpoint
        build full current-mesh before and after render contexts

        weightedLoss = 0
        foregroundMass = 0

        for each of six official views:
            project affectedFaces before and after
            crop their union with the exact 6-pixel SSIM support margin
            reject if crop side < 11 or > 384

            render full current mesh before and after into that crop
            normalSsim = mean(channel SSIM for nx, ny, nz)
            depthSsim = SSIM(depth)
            viewLoss = 1 - 0.5 * (normalSsim + depthSsim)

            viewForegroundWindows = count crop centers, excluding the
                five-pixel border, whose before or after center is foreground

            weightedLoss += viewLoss * viewForegroundWindows
            foregroundMass += viewForegroundWindows

        localLoss = weightedLoss / foregroundMass
        rank = localLoss - currentStrikeDebtWeight * meanDebt

        force exactly the minimum-rank candidate from the unchanged legacy
        prefix and preserve every structural and coverage guard

    retain B01A strike counts and weights:
        T2: one strike, prefix 8, cap 0.30, debt weight 1e-5
        T3: four prefix-16 strikes at 1e-5,
            one prefix-8 strike at 1e-4,
            one prefix-8 strike at 5e-4

NON-TARGET BRANCHES
    Preserve the exact Push 21B schedules. The ranking implementation is
    generic, but it is reached only by existing strategic weld call sites.

HARD INVARIANTS
    identical to B01A
*/
