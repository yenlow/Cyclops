#' Oxford
#'
#' A dataset containing the MMR vaccination / meningitis in Oxford example from
#' Farrington and Whitaker.  There are 10 patients comprising 38 unique exposure intervals.
#'
#' @name oxford
#' 
#' @docType data
#' 
#' @usage data(oxford)
#' 
#' @format A data frame with 38 rows and 6 variables:
#' \describe{
#'   \item{indiv}{patient identifier}
#'   \item{event}{number of events in interval}
#'   \item{interval}{interval length in days}
#'   \item{agegr}{age group}
#'   \item{exgr}{exposure group}
#'   \item{loginterval}{log interval length}
#'   ...
#' }
#' @source \url{http://statistics.open.ac.uk/sccs/r.htm}
NULL
